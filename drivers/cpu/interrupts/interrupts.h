#pragma once

#define INTERRUPT_TIMER_SPEED 1000

#define INTERRUPT_TIMER 0x20
#define INTERRUPT_PS2   0x21

#include <drivers/impl.h>

extern struct cpu_core;

typedef void*(*interrupt_handler_t)(struct cpu_core*, struct regs32*);

struct int_subscriber
{
	bool in_use;
	interrupt_handler_t handler;
};

void cpu_ints_init();
void cpu_ints_core_init(struct cpu_core *core);
void cpu_ints_core_disable(struct cpu_core *core);

void cpu_ints_sub(int int_no, interrupt_handler_t handler);
void cpu_ints_unsub(interrupt_handler_t handler);
