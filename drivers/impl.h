#pragma once

// implementation for the x86_64 arch

#include <kernel/include/C/typedefs.h>

#define CPUID_SVM_FEATURE_BIT (1 << 2)
#define CPUID_SVM_SVML (1 << 2)

#define AMD_VM_CR 0xC0010114
#define AMD_VM_CR_SVMDIS (1 << 4)

#define IA32_MISC_ENABLE 0x000001A0
#define IA32_APIC_BASE 0x0000001B

#define IA32_APIC_BASE_ENABLE 0x800
#define IA32_MISC_ENABLE_LCMV 0x400000

#define IA32_EFER 0xC0000080
#define IA32_EFER_SVME (1 << 12)

#define VM_HSAVE_PA 0xC0010117


/*
 * Note: Not complete implementation, must not be used.
 *
 * Calls cpuid instruction for leaf that require IA32_MISC_ENABLE.LCMV to be cleared to 0 first
*/
#define __cpuid_lcmv(leah, eax, ebx, ecx, edx)  \
    {                                                \
        uint32_t hi, lo0, lo1;                       \
        __get_msr(IA32_MISC_ENABLE, &lo0, &hi); \
        lo1 = lo0;                                   \
        lo0 &= ~(IA32_MISC_ENABLE_LCMV);             \
        __set_msr(IA32_MISC_ENABLE, lo0, hi);   \
        __cpuid(leah, eax, ebx, ecx, edx);      \
        __set_msr(IA32_MISC_ENABLE, lo1, hi);   \
    }

#define __enable_int() __asm__ volatile("sti")
#define __disable_int() __asm__ volatile("cli")
#define __halt() __asm__ volatile("hlt")
#define __load_idt(descriptor) __asm__ volatile("lidt %0" :: "m"(descriptor))

#define __io_wait() __outb(0x80, 0)

struct regs32
{
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss;
} __attribute__((packed));

struct regs16
{
    unsigned short di, si, bp, sp, bx, dx, cx, ax;
    unsigned short gs, fs, es, ds, eflags;
} __attribute__((packed));

#define LNG_PTR(seg, off) ((seg << 4) | off)
#define REAL_PTR(arr) LNG_PTR((arr)[1], (arr)[0])
#define SEG(addr) ((((uint32_t)addr) >> 4) & 0xF000)
#define OFF(addr) (((uint32_t)addr) & 0xFFFF)

void int32(unsigned char intnum, struct regs32 *regs);

static inline void __outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1"
                 :
                 : "a"(val), "Nd"(port));
}

static inline void __outw(uint16_t port, uint16_t val)
{
    __asm__ volatile("outw %w0, %w1"
                     :
                     : "a"(val), "Nd"(port));
}

static inline void __outsw(unsigned short int __port, const void *__addr,
                                unsigned long int __count)
{
    __asm__ __volatile__("cld ; rep ; outsw"
                         : "=S"(__addr), "=c"(__count)
                         : "d"(__port), "0"(__addr), "1"(__count));
}

static inline void __insw(unsigned short int __port, void *__addr, unsigned long int __count)
{
    __asm__ __volatile__("cld ; rep ; insw"
                         : "=D"(__addr), "=c"(__count)
                         : "d"(__port), "0"(__addr), "1"(__count));
}

static inline uint8_t __inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0"
                 : "=a"(ret)
                 : "Nd"(port));
    return ret;
}

static inline uint16_t __inw(uint16_t port)
{
    uint16_t data;
    __asm__ volatile("inw %w1, %w0"
                     : "=a"(data)
                     : "Nd"(port));
    return data;
}

static inline void __get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    __asm__ volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

static inline void __set_msr(uint32_t msr, uint32_t lo, uint32_t hi)
{
    __asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

static inline void __rdtsc(uint32_t *lo, uint32_t *hi)
{
    __asm__ volatile("rdtsc" : "=a"(*lo), "=d"(*hi));
}

// tells are interrupts enabled
static inline bool __eint()
{
    unsigned long flags;
    __asm__ volatile("pushf\n\t"
                 "pop %0"
                 : "=g"(flags));
    return flags & (1 << 9);
}

static inline void __cpuid(uint32_t leah, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "0"(leah));
}
