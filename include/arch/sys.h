#ifndef _INARI_SYS_H
#define _INARI_SYS_H

#define REGISTER_IP 0x00
#define REGISTER_SP 0x01

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
#define modify_register_array(array, reg, data) _arch_modify_register_array(array, reg, (uintptr_t)data)
#define trigger_interrupt(int) _arch_trigger_interrupt(int)

extern void _arch_disable_int(void);
extern void _arch_enable_int(void);
extern void _arch_hlt(void);
extern void _arch_idle(void);
extern uint32_t _arch_core_id(void);
extern void _arch_modify_register_array(void *array, uint16_t reg, uintptr_t data);
extern void _arch_trigger_interrupt(uint32_t interrupt);
extern uint32_t _arch_local_irq_save(void);
extern void _arch_local_irq_restore(uint32_t flags);

#endif