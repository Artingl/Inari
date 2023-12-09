#pragma once

#include <kernel/kernel.h>

#include <kernel/include/C/math.h>
#include <kernel/include/C/typedefs.h>

#define IOREGSEL 0x00
#define IOREGWIN 0x10

#define IOAPICID          0x00
#define IOAPICVER         0x01
#define IOAPICARB         0x02
#define IOAPICREDTBL(n)   (0x10 + n * 2)

// #define IO_APIC_BASE align(0xc0004000, PAGE_SIZE)

void cpu_io_apic_init(uintptr_t ioapic);
void cpu_io_apic_map(uint8_t index, uint64_t value);
void cpu_io_apic_disable();

void cpu_io_apic_write_reg(uint8_t reg, uint32_t value);
uint32_t cpu_io_apic_read_reg(uint8_t reg);

uintptr_t cpu_io_apic_get_base();
