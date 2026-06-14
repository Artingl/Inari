#ifndef _INARI_SWI_H
#define _INARI_SWI_H

#include <kernel/interrupts/interrupts.h>
#include <misc/types.h>

#define SWI_RESCHEDULE 0x2000
#define SWI_SYSCALL    0x2080

#define SWI_HANDLED 1

typedef int (*swi_handler_t)(uint32_t swi, void *driver_data);

int swi_request(uint32_t swi, swi_handler_t handler, void *driver_data);
int swi_free(uint32_t swi, swi_handler_t handler);
void swi_dispatch(struct interrupt_frame frame);

#endif