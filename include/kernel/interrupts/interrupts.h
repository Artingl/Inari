#ifndef _INARI_INTERRUPTS_H
#define _INARI_INTERRUPTS_H

#include <misc/types.h>

struct interrupt_registers
{
    void *base;
    size_t size;
};

struct interrupt_frame
{
    uint32_t int_no;
    struct interrupt_registers registers;
};

extern void interrupts_trigger(uint32_t interrupt);
extern void interrupt_dispatch(struct interrupt_frame frame);
extern struct interrupt_frame *interrupt_get_frame();

#endif