#ifndef _INARI_PIC_H
#define _INARI_PIC_H

#include <misc/list.h>
#include <kernel/sys/driver.h>
#include <kernel/sys/device.h>

struct pci_device
{
    uint16_t device_id, vendor_id;
    uint16_t status, command;
    uint8_t class_code, subclass;
    uint8_t bus, slot, func;

    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;

    struct list_head list;
};

int pci_init();
int pci_get_device(struct pci_device **dev, uint8_t bus, uint8_t slot, uint8_t func);
int pci_find_next(struct pci_device **dev, uint16_t device_id, uint16_t vendor_id);

#endif
