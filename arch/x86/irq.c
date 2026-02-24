#include <kernel/inari.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/interrupts/irq.h>
#include <kernel/interrupts/swi.h>
#include <kernel/sys/syscall.h>

#include <arch/paging.h>
#include <arch/x86/pit.h>
#include <arch/x86/cpu.h>
#include <arch/x86/irq.h>
#include <arch/x86/interrupts.h>

#define DECL_DIRECT(n, flags) extern void _arch_irq##n(void);x86_cpu_install_idt(core->core_id, (unsigned)_arch_irq##n, n, 0x08, flags)
#define DECL_IRQ(n)           extern void _arch_irq##n(void);x86_cpu_install_idt(core->core_id, (unsigned)_arch_irq##n, 32 + n, 0x08, IDT_PRESENT | IDT_INT32_GATE)

void x86_irq_setup(struct x86_cpu *core)
{
    /* Note: don't forget to add the subroutines in interrupts.S !!! */
    DECL_IRQ(0);
    DECL_IRQ(1);
    DECL_IRQ(2);
    DECL_IRQ(3);
    DECL_IRQ(4);
    DECL_IRQ(5);
    DECL_IRQ(6);
    DECL_IRQ(7);
    DECL_IRQ(8);
    DECL_IRQ(9);
    DECL_IRQ(10);
    DECL_IRQ(11);
    DECL_IRQ(12);
    DECL_IRQ(13);
    DECL_IRQ(14);
    DECL_IRQ(15);

    /* SWI */
    DECL_DIRECT(0x80, IDT_PRESENT | IDT_INT32_GATE | IDT_DLP_USR);  // SWI_RESCHEDULE
    DECL_DIRECT(0x81, IDT_PRESENT | IDT_INT32_GATE | IDT_DLP_USR);  // SWI_SYSCALL
}

void x86_irq_handler(struct x86_regs32 *regs)
{
    arch_switch_pagedir(arch_get_kernel_pagedir());
    uint32_t irq = regs->int_no;

    if (regs->int_no == X86_PIT_IRQ)
    {
        x86_pit_irq();
        irq = IRQ_TIMER_INTERRUPT;
    }
    else if (regs->int_no == X86_SWI_RESCHEDULE) irq = SWI_RESCHEDULE;
    else if (regs->int_no == X86_SWI_SYSCALL) {
        regs->ebx = syscall_handle(regs->ebx, (void*)regs->ecx, (void*)regs->edx, (void*)regs->esi, (void*)regs->edi, (void*)regs->ebp);
        irq = SWI_SYSCALL;

        /* Restore kernel pagedir just in case */
        arch_switch_pagedir(arch_get_kernel_pagedir());
    }
    else irq -= X86_IRQ_OFFSET;

    interrupt_dispatch((struct interrupt_frame){
        .int_no = irq,
        .registers = {
            .base = regs,
            .size = sizeof(struct x86_regs32)
        }
    });
    x86_cpu_ack_irq(regs->int_no);
}

