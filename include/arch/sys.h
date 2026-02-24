#ifndef _INARI_SYS_H
#define _INARI_SYS_H

#define local_irq_save(flags)      \
    do {                           \
        flags = _arch_local_irq_save(); \
    } while (0)

#define local_irq_restore(flags)      \
    do {                           \
        _arch_local_irq_restore(flags); \
    } while (0)

#define halt() _arch_hlt()
#define disable_int() _arch_disable_int()
#define enable_int() _arch_enable_int()
#define core_id() _arch_core_id()
#define idle() _arch_idle()
#define trigger_interrupt(int) _arch_trigger_interrupt(int)

void _arch_disable_int(void);
void _arch_enable_int(void);
void _arch_hlt(void);
void _arch_idle(void);
uint32_t _arch_core_id(void);
void _arch_trigger_interrupt(uint32_t interrupt);
uint32_t _arch_local_irq_save(void);
void _arch_local_irq_restore(uint32_t flags);

#endif