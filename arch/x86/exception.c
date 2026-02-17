#include <kernel/inari.h>
#include <kernel/proc/sched.h>
#include <kernel/interrupts/swi.h>
#include <kernel/proc/signals.h>
#include <kernel/console/console.h>

#include <arch/x86/cpu.h>
#include <arch/x86/arch.h>
#include <arch/x86/exception.h>

#define DECL_EXCP(n) extern void _arch_excp##n(void);x86_cpu_install_idt(core->core_id, (unsigned)_arch_excp##n, n, 0x08, 0x8e)

static const char *EXCEPTIONS_NAMES[] =
{
    [ 0x0 ] = "Division_Error",
    [ 0x1 ] = "Debug_Exception",
    [ 0x2 ] = "Non_Maskable_Interrupt",
    [ 0x3 ] = "Breakpoint_Triggered",
    [ 0x4 ] = "Overflow_Error",
    [ 0x5 ] = "Bound_Range_Exceeded",
    [ 0x6 ] = "Invalid_Opcode",
    [ 0x7 ] = "Device_Not_Available",
    [ 0x8 ] = "Double_Fault",
    [ 0x9 ] = "Cross_Segment_Overrun",
    [ 0xa ] = "Invalid_TSS",
    [ 0xb ] = "Segment_Not_Present",
    [ 0xc ] = "Stack_Segment_Fault",
    [ 0xd ] = "General_Protection_Fault",
    [ 0xe ] = "Page_Fault",
    [ 0x10 ] = "FP_Exception",
    [ 0x11 ] = "Alignment_Check",
    [ 0x12 ] = "Machine_Check",
    [ 0x13 ] = "SIMD_Float_Exception",
    [ 0x14 ] = "Virtualization_Fault",
    [ 0x15 ] = "Control_Protection_Violation",
    [ 0x1d ] = "Hypervisor_Injection",
    [ 0x1e ] = "Security_Violation",
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
    console_switch_early();
    tid_t tid;
    uint32_t signo = SIGTERM;
    if (sched_current_thread(&tid) == 0)
    {
        switch (regs.int_no)
        {
            case 0xe:  signo = SIGSEGV; break;
            case 0x10: signo = SIGFPE; break;
            case 0x6:  signo = SIGILL; break;
            case 0x0:  signo = SIGILL; break;
            default:
                printk("arch: exception %s; eip=0x%x", EXCEPTIONS_NAMES[regs.int_no], regs.eip);
        }
        
        sched_signal_thread(tid, signo);
    }
    else {
        /* Exception in kernel code */
        panic("kernel exception %s", EXCEPTIONS_NAMES[regs.int_no]);
    }

    /* Use interrupt dispatcher to reschedule */
    interrupt_dispatch((struct interrupt_frame){
        .int_no = SWI_RESCHEDULE,
        .registers = {
            .base = &regs,
            .size = sizeof(struct x86_regs32)
        }
    });

    console_switch_normal();
}

