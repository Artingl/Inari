#include <kernel/arch/i386/impl.h>
#include <kernel/lock/spinlock.h>

spinlock_t io_spinlock = {0};

void __outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1"
                 :
                 : "a"(val), "Nd"(port));
}

void __outw(uint16_t port, uint16_t val)
{
    __asm__ volatile("outw %w0, %w1"
                     :
                     : "a"(val), "Nd"(port));
}

void __outsw(unsigned short int __port, const void *__addr,
                                unsigned long int __count)
{
    __asm__ __volatile__("cld ; rep ; outsw"
                         : "=S"(__addr), "=c"(__count)
                         : "d"(__port), "0"(__addr), "1"(__count));
}

void __insw(unsigned short int __port, void *__addr, unsigned long int __count)
{
    __asm__ __volatile__("cld ; rep ; insw"
                         : "=D"(__addr), "=c"(__count)
                         : "d"(__port), "0"(__addr), "1"(__count));
}

uint8_t __inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0"
                 : "=a"(ret)
                 : "Nd"(port));
    return ret;
}

uint16_t __inw(uint16_t port)
{
    uint16_t data;
    __asm__ volatile("inw %w1, %w0"
                     : "=a"(data)
                     : "Nd"(port));
    return data;
}

void __get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    __asm__ volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void __set_msr(uint32_t msr, uint32_t lo, uint32_t hi)
{
    __asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

void __rdtsc(uint32_t *lo, uint32_t *hi)
{
    __asm__ volatile("rdtsc" : "=a"(*lo), "=d"(*hi));
}

// tells are interrupts enabled
bool __eint()
{
    unsigned long flags;
    __asm__ volatile("pushf\n\t"
                 "pop %0"
                 : "=g"(flags));
    return flags & (1 << 9);
}

void __cpuid(uint32_t leah, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "0"(leah));
}
