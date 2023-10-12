#pragma once

#define INTERRUPT_TIMER_SPEED 1000

#define INTERRUPT_TIMER 0x20
#define INTERRUPT_PS2   0x21

#include <drivers/impl.h>

typedef void(*interrupt_handler_t)(struct regs32*);

struct int_subscriber
{
	bool in_use;
	interrupt_handler_t handler;
};

void cpu_interrupts_init();
void cpu_interrupts_disable();

void cpu_interrupts_unsubscribe(uint8_t int_no, int32_t id);
int32_t cpu_interrupts_subscribe(interrupt_handler_t handler, uint8_t int_no);
