#ifndef _INARI_SWI_H
#define _INARI_SWI_H

#include <misc/types.h>
#include <kernel/interrupts/interrupts.h>

#define SWI_RESCHEDULE 0x2000
#define SWI_SYSCALL    0x2080

#define SWI_HANDLED 1

typedef int (*swi_handler_t)(uint32_t swi, void *dev_id);

int swi_request(uint32_t swi, swi_handler_t handler, void *dev_id);
int swi_free(uint32_t swi, swi_handler_t handler);
void swi_dispatch(struct interrupt_frame frame);

#endif