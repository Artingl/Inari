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
#define __halt() do { __asm__ volatile("cli\nhlt"); } while (1)
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

typedef struct regs32 regs32_t;
typedef struct regs16 regs16_t;

#define LNG_PTR(seg, off) ((seg << 4) | off)
#define REAL_PTR(arr) LNG_PTR((arr)[1], (arr)[0])
#define SEG(addr) ((((uint32_t)addr) >> 4) & 0xF000)
#define OFF(addr) (((uint32_t)addr) & 0xFFFF)

void int32(unsigned char intnum, struct regs16 *regs);

void __outb(uint16_t port, uint8_t val);
void __outw(uint16_t port, uint16_t val);
void __outsw(unsigned short int __port, const void *__addr,
                                unsigned long int __count);
void __insw(unsigned short int __port, void *__addr, unsigned long int __count);
uint8_t __inb(uint16_t port);
uint16_t __inw(uint16_t port);
void __get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi);
void __set_msr(uint32_t msr, uint32_t lo, uint32_t hi);
void __rdtsc(uint32_t *lo, uint32_t *hi);

// tells are interrupts enabled
int __eint();
void __cpuid(uint32_t leah, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
