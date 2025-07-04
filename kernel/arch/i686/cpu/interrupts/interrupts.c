#include <kernel/kernel.h>
#include <kernel/libc/string.h>
#include <kernel/list/dynlist.h>
#include <kernel/driver/interrupt/interrupt.h>
#include <kernel/driver/serial/serial.h>

#include <kernel/arch/i686/impl.h>
#include <kernel/arch/i686/cpu/cpu.h>
#include <kernel/arch/i686/cpu/acpi/acpi.h>
#include <kernel/arch/i686/cpu/timers/pit.h>
#include <kernel/arch/i686/cpu/timers/apic_timer.h>
#include <kernel/arch/i686/cpu/interrupts/interrupts.h>
#include <kernel/arch/i686/cpu/interrupts/exceptions/exceptions.h>
#include <kernel/arch/i686/cpu/interrupts/pic/pic.h>
#include <kernel/arch/i686/cpu/interrupts/irq/irq.h>
#include <kernel/arch/i686/cpu/interrupts/apic/local_apic.h>
#include <kernel/arch/i686/cpu/interrupts/apic/io_apic.h>

void cpu_interrupts_core_init(struct cpu_core *core)
{
    cpu_interrupts_idt_init(core);
    
    // initialize exceptions and IRQs
    cpu_exceptions_core_init(core);
    cpu_irq_init(core);
#ifdef CONFIG_I686_NOAPIC
    printk("ints: using PIC only because CONFIG_I686_NOAPIC option was set during compilation.");
    cpu_pic_init();
    cpu_pit_init();
#else
    // initialize APIC or PIC (APIC: edx APIC feature flag is set && ACPI is loaded ; Otherwise PIC)
    if (!(cpu_feat_edx() & CPU_FEATURE_EDX_APIC) || !cpu_acpi_loaded())
    {
        printk("ints: CPU does not support APIC, or we had an error initializing it. Using PIC instead.");
        cpu_pic_init();
        cpu_pit_init();
    }
    else
    {
        cpu_lapic_init(core);
        if (core->is_bsp)
        {
            cpu_atimer_init(core);
            cpu_pit_disable();
        }
    }
#endif

    core->ints_loaded = 1;
}

void cpu_interrupts_core_disable(struct cpu_core *core)
{
    core->ints_loaded = 0;
    // Disable APIC/PIC
    if (cpu_using_apic())
        cpu_lapic_disable(core);
    else
        cpu_pic_disable();
}

void cpu_interrupts_idt_init(struct cpu_core *core)
{
    core->idt_desc.size = (sizeof(struct cpu_idt) * 256) - 1;
    core->idt_desc.base = (uintptr_t)core->idt;
    printk("idt: base ptr %p", (unsigned long)core->idt);
}

void cpu_interrupts_idt_install(struct cpu_core *core, unsigned long base, uint8_t num, uint16_t sel, uint8_t flags)
{
    core->idt[num].base_low = ((uint64_t)base & 0xFFFF);
    core->idt[num].base_high = ((uint64_t)base >> 16) & 0xFFFF;
    core->idt[num].sel = sel;
    core->idt[num].zero = 0;
    core->idt[num].flags = flags | 0x60;
}

uint32_t arch_syscall(uint8_t id, uint32_t param0, uint32_t param1, uint32_t param2)
{
    uint32_t syscall_result = 0;
    __asm__ volatile(
        "push %%ebx\n"
        "push %%ecx\n"
        "push %%edx\n"
        "mov %0, %%eax\n"
        "mov %1, %%ebx\n"
        "mov %2, %%ecx\n"
        "mov %3, %%edx\n"
        "int $0x80\n"
        :
        : "m"(id), "m"(param0), "m"(param1), "m"(param2)
    );

    __asm__ volatile(
        "mov %%eax, %0\n"
        "pop %%edx\n"
        "pop %%ecx\n"
        "pop %%ebx\n"
        : "=r"(syscall_result)
    );

    return syscall_result;
}

extern uint32_t kern_interrupts_syscall_handle(uint8_t id, uint32_t param0, uint32_t param1, uint32_t param2, void *regs_ptr);
extern void kern_interrupts_arch_handle(uint8_t int_no, void *regs_ptr);

uintptr_t isr_handler(struct regs32 *regs)
{
    struct cpu_core *core = cpu_current_core();

    if (regs->int_no < 32)
    {
        // Handle exceptions
        cpu_exceptions_core_handle(core, regs);
    }
    else if (regs->int_no >= 32 && regs->int_no <= 47 && !cpu_using_apic())
    {
        // Acknowledge the IRQ
        cpu_irq_acknowledge(regs->int_no - 32);
    }

    // write EOI if using APIC
    if (cpu_using_apic())
    {
        cpu_lapic_out(core, LAPIC_EOI, 0x0);
    }

    // Forward the interrupt to the kernel's interrupt handler using the IDs that the kernel understands
    if (regs->int_no == INTERRUPT_TIMER)
        kern_interrupts_arch_handle(KERN_INTERRUPT_TIMER, regs);
    else if (regs->int_no == INTERRUPT_PS2)
    {
        kern_interrupts_arch_handle(KERN_INTERRUPT_PS2, regs);
        printk("key");
        __outb(0x64, __inb(0x60));
    }
    else if (regs->int_no == INTERRUPT_SYSCALL)
    {
        regs->eax = kern_interrupts_syscall_handle(regs->eax, regs->ebx, regs->ecx, regs->edx, regs);
    }
    else
    {
        // If we don't know the source and/or ID of the interrupt, send it as it is
        kern_interrupts_arch_handle(regs->int_no, regs);
    }

    return 0;
}
