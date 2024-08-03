#pragma once

#include <kernel/kernel.h>

#include <kernel/include/C/math.h>
#include <kernel/include/C/typedefs.h>

#include <kernel/arch/i386/cpu/cpu.h>

#define IA32_APIC_BASE_BSP              0x100 // Processor is a BSP

#define LAPIC_ID                        0x0020  // Local APIC ID
#define LAPIC_VER                       0x0030  // Local APIC Version
#define LAPIC_TPR                       0x0080  // Task Priority
#define LAPIC_APR                       0x0090  // Arbitration Priority
#define LAPIC_PPR                       0x00a0  // Processor Priority
#define LAPIC_EOI                       0x00b0  // EOI
#define LAPIC_RRD                       0x00c0  // Remote Read
#define LAPIC_LDR                       0x00d0  // Logical Destination
#define LAPIC_DFR                       0x00e0  // Destination Format
#define LAPIC_SVR                       0x00f0  // Spurious Interrupt Vector
#define LAPIC_ISR                       0x0100  // In-Service (8 registers)
#define LAPIC_TMR                       0x0180  // Trigger Mode (8 registers)
#define LAPIC_IRR                       0x0200  // Interrupt Request (8 registers)
#define LAPIC_ESR                       0x0280  // Error Status
#define LAPIC_ICRLO                     0x0300  // Interrupt Command
#define LAPIC_ICRHI                     0x0310  // Interrupt Command [63:32]
#define LAPIC_TIMER                     0x0320  // LVT Timer
#define LAPIC_THERMAL                   0x0330  // LVT Thermal Sensor
#define LAPIC_PERF                      0x0340  // LVT Performance Counter
#define LAPIC_LINT0                     0x0350  // LVT LINT0
#define LAPIC_LINT1                     0x0360  // LVT LINT1
#define LAPIC_ERROR                     0x0370  // LVT Error
#define LAPIC_TICR                      0x0380  // Initial Count (for Timer)
#define LAPIC_TCCR                      0x0390  // Current Count (for Timer)
#define LAPIC_TDCR                      0x03e0  // Divide Configuration (for Timer)

// #define LAPIC_BASE align(0xc000a000, PAGE_SIZE)

// will return amount of physical cpus
void cpu_lapic_init(struct cpu_core *core);
void cpu_lapic_disable(struct cpu_core *core);
void cpu_lapic_out(struct cpu_core *core, uint32_t addr, uint32_t value);
void cpu_lapic_set_base(struct cpu_core *core, uintptr_t base);

uint32_t cpu_lapic_in(struct cpu_core *core, uint32_t addr);
uintptr_t cpu_lapic_get_base(struct cpu_core *core);
