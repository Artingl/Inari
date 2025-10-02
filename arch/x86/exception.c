#include <kernel/inari.h>

#include <arch/x86/cpu.h>
#include <arch/x86/exception.h>

#define DECL_EXCP(n) extern void _arch_excp##n(void);x86_cpu_install_idt(core->core_id, (unsigned)_arch_excp##n, n, 0x08, 0x8e)

static const char *EXCEPTIONS_NAMES[] =
{
    [ 0x0 ] = "DIVISION_ERROR_EXCEPTION",
    [ 0x1 ] = "DEBUG_EXCEPTION",
    [ 0x2 ] = "NON_MASKABLE_INTERRUPT_EXCEPTION",
    [ 0x3 ] = "BREAKPOINT_EXCEPTION",
    [ 0x4 ] = "OVERFLOW_EXCEPTION",
    [ 0x5 ] = "BOUND_RAGE_EXCEEDED_EXCEPTION",
    [ 0x6 ] = "INVALID_OPCODE_EXCEPTION",
    [ 0x7 ] = "DEVICE_NOT_AVAILABLE_EXCEPTION",
    [ 0x8 ] = "DOUBLE_FAULT_EXCEPTION",
    [ 0x9 ] = "CROSS_SEGMENT_OVERRRUN_EXCEPTION",
    [ 0xa ] = "INVALID_TSS_EXCEPTION",
    [ 0xb ] = "SEGMENT_NOT_PRESENT_EXCEPTION",
    [ 0xc ] = "STACK_SEGMENT_FAULT_EXCEPTION",
    [ 0xd ] = "GENERAL_PROTECTION_FAULT_EXCEPTION",
    [ 0xe ] = "PAGE_FAULT_EXCEPTION",
    [ 0x10 ] = "x87_FLOATING_POINT_EXCEPTION",
    [ 0x11 ] = "ALIGNMENT_CHECK_EXCEPTION",
    [ 0x12 ] = "MACHINE_CHECK_EXCEPTION",
    [ 0x13 ] = "SIMD_FLOATING_POINT_EXCEPTION",
    [ 0x14 ] = "VIRTUALIZATION_EXCEPTION",
    [ 0x15 ] = "CONTROL_PROTECTION_EXCEPTION",
    [ 0x1d ] = "HYPERVISOR_INJECTION_EXCEPTION",
    [ 0x1e ] = "SECURITY_EXCEPTION",
};

void x86_exception_setup(struct x86_cpu *core)
{
    DECL_EXCP(0);
    DECL_EXCP(1);
    DECL_EXCP(2);
    DECL_EXCP(3);
    DECL_EXCP(4);
    DECL_EXCP(5);
    DECL_EXCP(6);
    DECL_EXCP(7);
    DECL_EXCP(8);
    DECL_EXCP(9);
    DECL_EXCP(10);
    DECL_EXCP(11);
    DECL_EXCP(12);
    DECL_EXCP(13);
    DECL_EXCP(14);
    DECL_EXCP(16);
    DECL_EXCP(17);
    DECL_EXCP(18);
    DECL_EXCP(19);
    DECL_EXCP(20);
    DECL_EXCP(21);
    DECL_EXCP(28);
    DECL_EXCP(29);
    DECL_EXCP(30);
}

void x86_exception_handler(struct x86_regs32 regs)
{
    panic("exception %s", EXCEPTIONS_NAMES[regs.int_no]);
}

