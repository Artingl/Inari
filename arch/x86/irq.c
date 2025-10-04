#include <kernel/inari.h>
#include <kernel/irq/irq.h>

#include <arch/x86/cpu.h>
#include <arch/x86/irq.h>

#define DECL_IRQ(n) extern void _arch_irq##n(void);x86_cpu_install_idt(core->core_id, (unsigned)_arch_irq##n, 32 + n, 0x08, 0x8e)

#define X86_IRQ_OFFSET 32
#define X86_PIT_IRQ    (X86_IRQ_OFFSET + 0)

void x86_irq_setup(struct x86_cpu *core)
{
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
}

void x86_irq_handler(struct x86_regs32 regs)
{
    uint32_t irq = regs.int_no;

    if (regs.int_no == X86_PIT_IRQ) irq = IRQ_TIMER_INTERRUPT;
    else irq -= X86_IRQ_OFFSET;

    irq_dispatch((struct interrupt_frame){
        .int_no = irq
    });
    x86_cpu_acknowledge_irq(regs.int_no);
}

