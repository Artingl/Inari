#ifdef CONFIG_SUBSYS_PCI

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/vmm.h>
#include <kernel/subsys/pci.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>

#include <arch/paging.h>
#include <arch/sys.h>

#include <misc/list.h>

static LIST_HEAD(pci_devices);

static struct char_ops ops = {};

static void read_bar(struct pci_device *dev, uint8_t bar) {
    uint8_t offsets[] = {0x10, 0x14, 0x18, 0x1C, 0x20, 0x24};
    uint32_t orig_cmd;
    if (!dev || offsets[bar] == 0 || dev->bar[bar].type != 0)
        return;
    dev->bar[bar].type = 0;
    dev->bar[bar].is_x64 = 0;

    /* Disable I/O and Memory decode */
    orig_cmd = dev->command;
    dev->command &= ~PCI_CMD_IO_SPACE;
    dev->command &= ~PCI_CMD_MEM_SPACE;
    pci_write_32(dev->bus, dev->slot, dev->func, 0x04, ((uint32_t)dev->status << 16) | dev->command);

    dev->bar[bar].base = pci_read_32(dev->bus, dev->slot, dev->func, offsets[bar]);
    dev->bar[bar].original = dev->bar[bar].base;
    if (dev->bar[bar].base & 1) {
        dev->bar[bar].base &= ~0b1111;
        dev->bar[bar].type = PCI_TYPE_IO_SPACE;
    } else {
        dev->bar[bar].base &= ~0b11;
        dev->bar[bar].type = PCI_TYPE_MEMORY_SPACE;
        if ((dev->bar[bar].base & 0b110) == 0b100) {
            dev->bar[bar].is_x64 = 1;
            if (bar < 6) {
                dev->bar[bar + 1].is_x64 = 2;
                dev->bar[bar + 1].type = PCI_TYPE_MEMORY_SPACE;
                /* TODO: 64 bit addresses */
            }
        }
    }

    /* Determine bar size */
    pci_write_32(dev->bus, dev->slot, dev->func, offsets[bar], 0xffffffff);
    dev->bar[bar].size = ~(pci_read_32(dev->bus, dev->slot, dev->func, offsets[bar]) &
                           ~(dev->bar[bar].type == PCI_TYPE_IO_SPACE ? 0b11 : 0b1111)) +
                         1;
    pci_write_32(dev->bus, dev->slot, dev->func, offsets[bar], dev->bar[bar].original);

    /* Determine if BAR is unused */
    if (dev->bar[bar].size == 0x00) {
        dev->bar[bar].type = 0;
        goto end;
    }

    /* Map the BAR to virtual memory if memory space  */
    if (dev->bar[bar].type == PCI_TYPE_MEMORY_SPACE) {
        dev->bar[bar].vbase = vmm_alloc_kernel(MAX(dev->bar[bar].size >> 12, 1) + 1);
        arch_map_page(arch_get_kernel_pagedir(), dev->bar[bar].vbase, (void *)dev->bar[bar].base, dev->bar[bar].size,
                      PAGE_RW | PAGE_PRESENT);

#ifdef CONFIG_DEBUG
        kprintf("pci: %d:%d:%d BAR %d%s MEM_SPACE size 0x%x base 0x%x", dev->bus, dev->slot, dev->func, bar,
                (dev->bar[bar].is_x64 ? " (64 bit)" : ""), dev->bar[bar].size, dev->bar[bar].vbase);
    } else {
        kprintf("pci: %d:%d:%d BAR %d%s IO_SPACE size 0x%x base 0x%x", dev->bus, dev->slot, dev->func, bar,
                (dev->bar[bar].is_x64 ? " (64 bit)" : ""), dev->bar[bar].size, dev->bar[bar].base);
#endif
    }

end:
    /* Restore command value */
    dev->command = orig_cmd;
    pci_write_32(dev->bus, dev->slot, dev->func, 0x04, ((uint32_t)dev->status << 16) | dev->command);
}

int pci_init() {
    struct pci_device *dev;
    uint32_t device_id, vendor_id;
    uint16_t class_code, subclass, header_type;
    uint16_t bus, slot, func;

    register_chardev_group(PCI_DRIVER, "pci");

    for (bus = 0; bus < 256; bus++) {
        for (slot = 0; slot < 32; slot++) {
            /* Read the Vendor ID at offset 0 */
            device_id = pci_read_32(bus, slot, 0, 0) >> 16;
            vendor_id = pci_read_32(bus, slot, 0, 0) & 0xFFFF;
            if (vendor_id == 0xFFFF)
                continue;

            class_code = pci_read_32(bus, slot, 0, 0x08) >> 24;
            subclass = (pci_read_32(bus, slot, 0, 0x08) >> 16) & 0xff;
            header_type = (pci_read_32(bus, slot, 0, 0x0C) >> 16) & 0xff;

            /* Ignore PCI-to-PCI and PCI-to-CardBus bridges */
            if (header_type != 0)
                continue;
            func = 0; // header_type == 0 implies the device has only single func

            dev = (struct pci_device *)kmalloc(sizeof(struct pci_device));
            dev->device_id = device_id;
            dev->vendor_id = vendor_id;
            dev->class_code = class_code;
            dev->subclass = subclass;
            dev->irq = pci_read_32(bus, slot, func, 0x3C) & 0xff;
            dev->command = pci_read_32(bus, slot, func, 0x4) & 0xffff;
            dev->status = (pci_read_32(bus, slot, func, 0x4) >> 16) & 0xffff;
            dev->bus = bus;
            dev->slot = slot;
            dev->func = func;
            read_bar(dev, 0);
            read_bar(dev, 1);
            read_bar(dev, 2);
            read_bar(dev, 3);
            read_bar(dev, 4);
            read_bar(dev, 5);

            kprintf("pci: found device %x:%x at %d:%d:%d; irq %u", vendor_id, device_id, bus, slot, func, dev->irq);

            list_add(&dev->list, &pci_devices);
            register_chardev(PCI_DRIVER, &ops, dev, NULL);
        }
    }
    return 0;
}

int pci_update_registers(struct pci_device *dev) {
    if (!dev)
        return -ENODEV;
    pci_write_32(dev->bus, dev->slot, dev->func, 0x04, ((uint32_t)dev->status << 16) | dev->command);
    return 0;
}

int pci_find_next(struct pci_device **dev, uint16_t device_id, uint16_t vendor_id) {
    struct pci_device *entry;
    struct list_head *pos;

    list_for_each(pos, &pci_devices) {
        entry = list_entry(pos, struct pci_device, list);
        if (entry->device_id == device_id && entry->vendor_id == vendor_id) {
            /* Check that we don't return the same device */
            if (dev && *dev &&
                (entry->bus == (*dev)->bus && entry->slot == (*dev)->slot && entry->func == (*dev)->func))
                continue;

            if (dev)
                *dev = entry;
            return 0;
        }
    }

    return -ENODEV;
}

int pci_get_device(struct pci_device **dev, uint8_t bus, uint8_t slot, uint8_t func) {
    struct pci_device *entry;
    struct list_head *pos;

    list_for_each(pos, &pci_devices) {
        entry = list_entry(pos, struct pci_device, list);
        if (entry->bus == bus && entry->slot == slot && entry->func == func) {
            if (dev)
                *dev = entry;
            return 0;
        }
    }

    return -ENODEV;
}

#endif
