#pragma once

#include <kernel/include/C/typedefs.h>

// Interrupt IDs
#define KERN_INTERRUPT_TIMER 0x00
#define KERN_INTERRUPT_PS2   0x01

#define KERN_INTERRUPT_EXCEPTION_PAGE_FAULT 0xff

typedef void(*interrupt_handler_t)();

int kern_interrupts_init();
int kern_interrupts_install_handler(uint8_t int_no, interrupt_handler_t handler);
int kern_interrupts_uninstall_handler(interrupt_handler_t handler);
