#include <kernel/kernel.h>

#include <drivers/cpu/interrupts/exceptions/exceptions.h>
#include <drivers/cpu/interrupts/idt/idt.h>

void cpu_int_excp_init()
{
    // install all exceptions
    cpu_idt_install((unsigned)_excp0, DIVISION_ERROR_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp1, DEBUG_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp2, NON_MASKABLE_INTERRUPT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp3, BREAKPOINT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp4, OVERFLOW_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp5, BOUND_RAGE_EXCEEDED_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp6, INVALID_OPCODE_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp7, DEVICE_NOT_AVAILABLE_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp8, DOUBLE_FAULT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp9, CROSS_SEGMENT_OVERRRUN_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp10, INVALID_TSS_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp11, SEGMENT_NOT_PRESENT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp12, STACK_SEGMENT_FAULT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp13, GENERAL_PROTECTION_FAULT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp14, PAGE_FAULT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp16, x87_FLOATING_POINT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp17, ALIGNMENT_CHECK_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp18, MACHINE_CHECK_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp19, SIMD_FLOATING_POINT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp20, VIRTUALIZATION_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp21, CONTROL_PROTECTION_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp29, HYPERVISOR_INJECTION_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install((unsigned)_excp30, SECURITY_EXCEPTION, 0x08, 0x8e);
}

extern void kernel_loop();

uintptr_t cpu_int_excp_handle(struct regs32 *regs)
{
    printk("CPU Exception: %s", EXCEPTIONS_NAMES[regs->int_no]);
    printk("\tGS  = 0x%x, FS  = 0x%x, ES  = 0x%x", regs->gs, regs->fs, regs->es);
    printk("\tEDI = 0x%x, ESI = 0x%x, EBP = 0x%x", regs->edi, regs->esi, regs->ebp);
    printk("\tESP = 0x%x, EBX = 0x%x, EDX = 0x%x", regs->esp, regs->ebp, regs->edx);
    printk("\tECX = 0x%x, EAX = 0x%x, INT = 0x%x", regs->ecx, regs->eax, regs->int_no);
    printk("\tERR = 0x%x, EIP = 0x%x, CS  = 0x%x", regs->err_code, regs->eip, regs->cs);
    printk("\tEFLAGS = 0x%x, USERSP = 0x%x, SS = 0x%x", regs->eflags, regs->useresp, regs->ss);

    // if (regs->int_no == PAGE_FAULT_EXCEPTION)
    //     return &kernel_loop;

    panic("^^^^^^^^^");    
}
