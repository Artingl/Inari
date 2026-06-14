#ifndef _INARI_PIC_H
#define _INARI_PIC_H

#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>
#include <misc/list.h>

#define PCI_CMD_BUS_MASTER (1 << 2)
#define PCI_CMD_MEM_SPACE  (1 << 1)
#define PCI_CMD_IO_SPACE   (1 << 0)

struct pci_device {
    uint16_t device_id, vendor_id;
    uint16_t status, command;
    uint8_t class_code, subclass;
    uint8_t bus, slot, func;
    uint8_t irq;

    struct {
#define PCI_TYPE_IO_SPACE     2
#define PCI_TYPE_MEMORY_SPACE 1
        uint8_t type;   // 0 - unused; 1 - memory space; 2 - IO space
        uint8_t is_x64; // 0 - 32 bit BAR; 1 - first part of 64 bit BAR; 2 - second part
        uint32_t size;
        uint32_t base;     // BAR base
        uint32_t original; // original BAR base with flags
        void *vbase;       // mapped virtual memory IF type is 1 (memory space)
    } bar[6];

    struct list_head list;
};

int pci_init();
int pci_update_registers(struct pci_device *dev);
int pci_get_device(struct pci_device **dev, uint8_t bus, uint8_t slot, uint8_t func);
int pci_find_next(struct pci_device **dev, uint16_t device_id, uint16_t vendor_id);

#endif
