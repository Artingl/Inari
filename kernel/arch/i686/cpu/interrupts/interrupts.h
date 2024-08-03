#pragma once

#define INTERRUPT_TIMER_SPEED 1000

#define INTERRUPT_TIMER 0x20
#define INTERRUPT_PS2   0x21

#include <kernel/arch/i686/impl.h>

struct cpu_core;

void cpu_interrupts_core_init(struct cpu_core *core);
void cpu_interrupts_core_disable(struct cpu_core *core);
void cpu_interrupts_idt_init(struct cpu_core *core);
void cpu_interrupts_idt_install(struct cpu_core *core, unsigned long base, uint8_t num, uint16_t sel, uint8_t flags);
