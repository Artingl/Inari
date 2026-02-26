#ifdef CONFIG_SUBSYS_PCI

#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/sys/pci.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>

#include <arch/paging.h>
#include <arch/sys.h>

#include <misc/list.h>

static LIST_HEAD(pci_devices);

static struct char_ops ops = {};

int pci_init()
{
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
            if (vendor_id == 0xFFFF) continue;
            
            class_code = pci_read_32(bus, slot, 0, 0x08) >> 24;
            subclass = (pci_read_32(bus, slot, 0, 0x08) >> 16) & 0xff;
            header_type = (pci_read_32(bus, slot, 0, 0x0C) >> 16) & 0xff;

            /* Ignore PCI-to-PCI and PCI-to-CardBus bridges */
            if (header_type != 0) continue;
            func = 0; // header_type == 0 implies the device has only single func

            kprintf("pci: found device %x:%x at %d:%d:%d", vendor_id, device_id, bus, slot, func);

            dev = (struct pci_device*)kmalloc(sizeof(struct pci_device));
            dev->device_id = device_id;
            dev->vendor_id = vendor_id;
            dev->class_code = class_code;
            dev->subclass = subclass;
            dev->bus = bus;
            dev->slot = slot;
            dev->func = func;
            dev->bar0 = pci_read_32(bus, slot, func, 0x10);
            dev->bar1 = pci_read_32(bus, slot, func, 0x14);
            dev->bar2 = pci_read_32(bus, slot, func, 0x18);
            dev->bar3 = pci_read_32(bus, slot, func, 0x1C);
            dev->bar4 = pci_read_32(bus, slot, func, 0x20);
            dev->bar5 = pci_read_32(bus, slot, func, 0x24);

            list_add(&dev->list, &pci_devices);

            register_chardev(PCI_DRIVER, &ops, dev, NULL);
        }
    }
    return 0;
}

int pci_find_next(struct pci_device **dev, uint16_t device_id, uint16_t vendor_id)
{
    struct pci_device *entry;
    struct list_head *pos;

    list_for_each(pos, &pci_devices) {
        entry = list_entry(pos, struct pci_device, list);
        if (entry->device_id == device_id && entry->vendor_id == vendor_id)
        {
            /* Check that we don't return the same device */
            if (dev && (entry->bus == (*dev)->bus || entry->slot == (*dev)->slot || entry->func == (*dev)->func))
                continue;

            if (dev) *dev = entry;
            return 0;
        }
    }

    return -1;
}

int pci_get_device(struct pci_device **dev, uint8_t bus, uint8_t slot, uint8_t func)
{
    struct pci_device *entry;
    struct list_head *pos;

    list_for_each(pos, &pci_devices) {
        entry = list_entry(pos, struct pci_device, list);
        if (entry->bus == bus && entry->slot == slot && entry->func == func)
        {
            if (dev) *dev = entry;
            return 0;
        }
    }

    return -1;
}

#endif