#ifndef _INARI_IRQ_H
#define _INARI_IRQ_H

#include <misc/types.h>
#include <kernel/interrupts/interrupts.h>

#define IRQ_TIMER_INTERRUPT 0x1000

#define IRQ_HANDLED 1

typedef int (*irq_handler_t)(uint32_t irq, void *dev_id);

extern int irq_request(uint32_t irq, irq_handler_t handler, void *dev_id);
extern int irq_free(uint32_t irq, void *dev_id);
extern void irq_dispatch(struct interrupt_frame frame);
extern struct interrupt_frame *irq_frame();

#endif