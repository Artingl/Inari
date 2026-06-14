#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_SUBSYS_NET
#ifdef CONFIG_SUBSYS_PCI
#ifdef CONFIG_DRV_E1000

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/module.h>
#include <kernel/subsys/pci.h>

#define INTEL_VEND    0x8086 // Vendor ID for Intel
#define E1000_DEV     0x100E // Device ID for the e1000 Qemu, Bochs, and VirtualBox emmulated NICs
#define E1000_I217    0x153A // Device ID for Intel I217
#define E1000_82577LM 0x10EA // Device ID for Intel 82577LM

static int total_devices = 0;
static struct pci_device *devices[8] = {0};

static void fetch_devid(uint16_t device_id) {
    struct pci_device *dev;
    size_t i;

    for (i = total_devices; i < 8; i++) {
        if (pci_find_next(&dev, device_id, INTEL_VEND) != 0)
            break;
        devices[total_devices++] = dev;
    }
}

static int e1000_probe() {
    fetch_devid(E1000_DEV);
    fetch_devid(E1000_I217);
    fetch_devid(E1000_82577LM);

    if (total_devices > 0)
        kprintf("e1000: initialized %d card(s).", total_devices);
    return total_devices == 0 ? -ENODEV : 0;
}

static void e1000_cleanup() {}

module_t e1000_module = {
    .probe = e1000_probe,
    .cleanup = e1000_cleanup,
};

module_register("e1000", e1000_module);

#endif
#endif
#endif
#endif
