#include <kernel/inari.h>
#include <kernel/interrupts/swi.h>
#include <kernel/interrupts/irq.h>

#include <arch/sys.h>
#include <arch/x86/cpu.h>
#include <arch/x86/interrupts.h>

void _arch_hlt(void)
{
    do { __asm__ volatile("cli\nhlt"); } while (1);
}

void _arch_disable_int(void)
{
    __asm__ volatile("cli");
}

void _arch_enable_int(void)
{
    __asm__ volatile("sti");
}

void x86_outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1"
                 :
                 : "a"(val), "Nd"(port));
}

void x86_outw(uint16_t port, uint16_t val)
{
    __asm__ volatile("outw %w0, %w1"
                     :
                     : "a"(val), "Nd"(port));
}

void x86_outsw(unsigned short int __port, const void *__addr,
                                unsigned long int __count)
{
    __asm__ __volatile__("cld ; rep ; outsw"
                         : "=S"(__addr), "=c"(__count)
                         : "d"(__port), "0"(__addr), "1"(__count));
}

void x86_insw(unsigned short int __port, void *__addr, unsigned long int __count)
{
    __asm__ __volatile__("cld ; rep ; insw"
                         : "=D"(__addr), "=c"(__count)
                         : "d"(__port), "0"(__addr), "1"(__count));
}

uint8_t x86_inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0"
                 : "=a"(ret)
                 : "Nd"(port));
    return ret;
}

uint16_t x86_inw(uint16_t port)
{
    uint16_t data;
    __asm__ volatile("inw %w1, %w0"
                     : "=a"(data)
                     : "Nd"(port));
    return data;
}

void x86_get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    __asm__ volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void x86_set_msr(uint32_t msr, uint32_t lo, uint32_t hi)
{
    __asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

void x86_rdtsc(uint32_t *lo, uint32_t *hi)
{
    __asm__ volatile("rdtsc" : "=a"(*lo), "=d"(*hi));
}

void x86_cpuid(uint32_t leah, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "0"(leah));
}

uint32_t _arch_core_id(void)
{
    uint32_t _0, _1, ebx, _2;
    x86_cpuid(1, &_0, &ebx, &_1, &_2);
    return ebx >> 24;
}

void _arch_idle(void)
{
    while (1)
    {
        __asm__ volatile("sti");
        __asm__ volatile("hlt");
    }
}

void _arch_modify_register_array(void *array, uint16_t reg, uintptr_t data)
{
    struct x86_regs32 *regs = (struct x86_regs32 *)array;

    switch (reg)
    {
        case REGISTER_IP:
            regs->eip = (uint32_t)data;
            return;
        case REGISTER_SP:
            regs->esp = (uint32_t)data;
            return;
        default:
            panic("x86: invalid register %u", reg);
    }
}

void _arch_trigger_interrupt(uint32_t interrupt)
{
    switch (interrupt)
    {
        case SWI_RESCHEDULE:
            __asm__ volatile ("int %0" : : "i" (X86_SWI_RESCHEDULE));
            return;
        case IRQ_TIMER_INTERRUPT:
            __asm__ volatile ("int %0" : : "i" (X86_PIT_IRQ));
            return;

        default:
            panic("x86: invalid interrupt to trigger %u", interrupt);
    }
}

uint32_t _arch_local_irq_save(void)
{
    unsigned long flags;
    asm volatile(
        "pushf\n\t"
        "pop %0\n\t"   /* save EFLAGS into flags */
        "cli"          /* clear IF bit (disable interrupts) */
        : "=rm"(flags)
        :
        : "memory");
    return flags;
}

void _arch_local_irq_restore(uint32_t flags)
{
    asm volatile(
        "push %0\n\t"
        "popf"
        :
        : "rm"(flags)
        : "memory", "cc");
}
