#pragma once

#define INTERRUPT_TIMER_SPEED 1000

#define INTERRUPT_TIMER 0x20
#define INTERRUPT_PS2   0x21

#include <kernel/arch/i686/impl.h>

struct cpu_core;

void cpu_ints_core_init(struct cpu_core *core);
void cpu_ints_core_disable(struct cpu_core *core);
