#pragma once

#include <drivers/impl.h>

#include <kernel/include/C/typedefs.h>

struct IDT
{
	unsigned short base_low;
	unsigned short sel;
	unsigned char zero;
	unsigned char flags;
	unsigned short base_high;
} __attribute__((packed));

struct IDT_ptr
{
	unsigned short size;
	uintptr_t base;
} __attribute__((packed));

extern void __cpu_idt_load();

void cpu_idt_init();
void cpu_idt_install(unsigned long base, uint8_t num, uint16_t sel, uint8_t flags);
void cpu_idt_init_memory();
