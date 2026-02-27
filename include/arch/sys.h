#ifndef _INARI_SYS_H
#define _INARI_SYS_H

#include <misc/types.h>

#define local_irq_save(flags)                                                                                          \
    do {                                                                                                               \
        flags = _arch_local_irq_save();                                                                                \
    } while (0)

#define local_irq_restore(flags)                                                                                       \
    do {                                                                                                               \
        _arch_local_irq_restore(flags);                                                                                \
    } while (0)

#define halt() _arch_hlt()
#define disable_int() _arch_disable_int()
#define enable_int() _arch_enable_int()
#define int_is_enabled() _arch_int_is_enabled()
#define core_id() _arch_core_id()
#define cpu_relax() _arch_cpu_relax()
#define trigger_interrupt(int) _arch_trigger_interrupt(int)
#define print_stacktrace(print) _print_stacktrace(NULL, print)
#define poweroff() _arch_poweroff()
#define reboot() _arch_reboot()
#define is_cpu_initialized() _arch_is_cpu_initalized()

#ifdef CONFIG_SUBSYS_PCI
#define pci_read_32(bus, slot, func, offset) _arch_pci_read_32(bus, slot, func, offset)
#define pci_write_32(bus, slot, func, offset, val) _arch_pci_write_32(bus, slot, func, offset, val)

uint32_t _arch_pci_read_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void _arch_pci_write_32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val);
#endif

int _arch_is_cpu_initalized(void);
void _arch_poweroff(void);
void _arch_reboot(void);
void _print_stacktrace(void *, void *print_handler);
int _arch_int_is_enabled(void);
void _arch_disable_int(void);
void _arch_enable_int(void);
void _arch_hlt(void);
void _arch_cpu_relax(void);
uint32_t _arch_core_id(void);
void _arch_trigger_interrupt(uint32_t interrupt);
uint32_t _arch_local_irq_save(void);
void _arch_local_irq_restore(uint32_t flags);

#endif