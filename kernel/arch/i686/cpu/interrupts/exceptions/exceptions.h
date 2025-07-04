#pragma once

#include <kernel/arch/i686/cpu/cpu.h>
#include <kernel/arch/i686/impl.h>

extern void _excp0();
extern void _excp1();
extern void _excp2();
extern void _excp3();
extern void _excp4();
extern void _excp5();
extern void _excp6();
extern void _excp7();
extern void _excp8();
extern void _excp9();
extern void _excp10();
extern void _excp11();
extern void _excp12();
extern void _excp13();
extern void _excp14();
extern void _excp16();
extern void _excp17();
extern void _excp18();
extern void _excp19();
extern void _excp20();
extern void _excp21();
extern void _excp29();
extern void _excp30();
extern void _excp31();

#define DIVISION_ERROR_EXCEPTION             0x0
#define DEBUG_EXCEPTION                      0x1
#define NON_MASKABLE_INTERRUPT_EXCEPTION     0x2
#define BREAKPOINT_EXCEPTION                 0x3
#define OVERFLOW_EXCEPTION                   0x4
#define BOUND_RAGE_EXCEEDED_EXCEPTION        0x5
#define INVALID_OPCODE_EXCEPTION             0x6
#define DEVICE_NOT_AVAILABLE_EXCEPTION       0x7
#define DOUBLE_FAULT_EXCEPTION               0x8
#define CROSS_SEGMENT_OVERRRUN_EXCEPTION     0x9
#define INVALID_TSS_EXCEPTION                0xa
#define SEGMENT_NOT_PRESENT_EXCEPTION        0xb
#define STACK_SEGMENT_FAULT_EXCEPTION        0xc
#define GENERAL_PROTECTION_FAULT_EXCEPTION   0xd
#define PAGE_FAULT_EXCEPTION                 0xe
#define x87_FLOATING_POINT_EXCEPTION         0x10
#define ALIGNMENT_CHECK_EXCEPTION            0x11
#define MACHINE_CHECK_EXCEPTION              0x12
#define SIMD_FLOATING_POINT_EXCEPTION        0x13
#define VIRTUALIZATION_EXCEPTION             0x14
#define CONTROL_PROTECTION_EXCEPTION         0x15
#define HYPERVISOR_INJECTION_EXCEPTION       0x1d
#define SECURITY_EXCEPTION                   0x1e

static const char *EXCEPTIONS_NAMES[] = {
    [ DIVISION_ERROR_EXCEPTION ] = "DIVISION_ERROR_EXCEPTION",
    [ DEBUG_EXCEPTION ] = "DEBUG_EXCEPTION",
    [ NON_MASKABLE_INTERRUPT_EXCEPTION ] = "NON_MASKABLE_INTERRUPT_EXCEPTION",
    [ BREAKPOINT_EXCEPTION ] = "BREAKPOINT_EXCEPTION",
    [ OVERFLOW_EXCEPTION ] = "OVERFLOW_EXCEPTION",
    [ BOUND_RAGE_EXCEEDED_EXCEPTION ] = "BOUND_RAGE_EXCEEDED_EXCEPTION",
    [ INVALID_OPCODE_EXCEPTION ] = "INVALID_OPCODE_EXCEPTION",
    [ DEVICE_NOT_AVAILABLE_EXCEPTION ] = "DEVICE_NOT_AVAILABLE_EXCEPTION",
    [ DOUBLE_FAULT_EXCEPTION ] = "DOUBLE_FAULT_EXCEPTION",
    [ CROSS_SEGMENT_OVERRRUN_EXCEPTION ] = "CROSS_SEGMENT_OVERRRUN_EXCEPTION",
    [ INVALID_TSS_EXCEPTION ] = "INVALID_TSS_EXCEPTION",
    [ SEGMENT_NOT_PRESENT_EXCEPTION ] = "SEGMENT_NOT_PRESENT_EXCEPTION",
    [ STACK_SEGMENT_FAULT_EXCEPTION ] = "STACK_SEGMENT_FAULT_EXCEPTION",
    [ GENERAL_PROTECTION_FAULT_EXCEPTION ] = "GENERAL_PROTECTION_FAULT_EXCEPTION",
    [ PAGE_FAULT_EXCEPTION ] = "PAGE_FAULT_EXCEPTION",
    [ x87_FLOATING_POINT_EXCEPTION ] = "x87_FLOATING_POINT_EXCEPTION",
    [ ALIGNMENT_CHECK_EXCEPTION ] = "ALIGNMENT_CHECK_EXCEPTION",
    [ MACHINE_CHECK_EXCEPTION ] = "MACHINE_CHECK_EXCEPTION",
    [ SIMD_FLOATING_POINT_EXCEPTION ] = "SIMD_FLOATING_POINT_EXCEPTION",
    [ VIRTUALIZATION_EXCEPTION ] = "VIRTUALIZATION_EXCEPTION",
    [ CONTROL_PROTECTION_EXCEPTION ] = "CONTROL_PROTECTION_EXCEPTION",
    [ HYPERVISOR_INJECTION_EXCEPTION ] = "HYPERVISOR_INJECTION_EXCEPTION",
    [ SECURITY_EXCEPTION ] = "SECURITY_EXCEPTION",
};

void cpu_exceptions_core_init(struct cpu_core *core);
void cpu_exceptions_core_handle(struct cpu_core *core, struct regs32 *regs);
