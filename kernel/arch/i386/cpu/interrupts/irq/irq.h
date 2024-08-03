#pragma once

#include <kernel/arch/i386/impl.h>
#include <kernel/arch/i386/cpu/cpu.h>
#include <kernel/include/C/typedefs.h>

#define IRQ_PIT   0x00
#define IRQ_PS2   0x01

typedef void(*irq_handler)(struct regs32 *);

void cpu_irq_init(struct cpu_core *core);
void cpu_irq_apic_remap();
void cpu_irq_acknowledge(uint8_t irq_no);
void cpu_irq_mask(unsigned char irq_line);
size_t cpu_irq_spurious_count();
