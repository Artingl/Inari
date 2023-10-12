#pragma once

#include <drivers/impl.h>

#include <kernel/include/C/typedefs.h>

#define IRQ_PIT   0x00
#define IRQ_PS2   0x01

typedef void(*irq_handler)(struct regs32 *);

void cpu_irq_init();
void cpu_irq_apic_remap();
void cpu_irq_acknowledge(uint8_t irq_no);
void cpu_irq_mask(unsigned char irq_line);
