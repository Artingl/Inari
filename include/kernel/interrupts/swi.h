#ifndef _INARI_SWI_H
#define _INARI_SWI_H

#include <misc/types.h>
#include <kernel/interrupts/interrupts.h>

#define SWI_RESCHEDULE 0x2000

#define SWI_HANDLED 1

typedef int (*swi_handler_t)(uint32_t swi, void *dev_id);

extern int swi_request(uint32_t irq, swi_handler_t handler, void *dev_id);
extern int swi_free(uint32_t irq, void *dev_id);
extern void swi_dispatch(struct interrupt_frame frame);

#endif