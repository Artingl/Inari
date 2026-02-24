#ifndef _INARI_X86_ARCH_H
#define _INARI_X86_ARCH_H

#define x86_io_wait() x86_outb(0x80, 0)

#include <kernel/proc/signals.h>

static uint32_t signal_exception[] =
{
    [ 0x10 ]  = SIGFPE,  // FP_Exception
    [ 0x6  ]  = SIGILL,  // Invalid_Opcode
    [ 0x0  ]  = SIGFPE,  // Division_Error
    [ 0x3  ]  = SIGTRAP, // Breakpoint_Triggered
    [ 0xc  ]  = SIGSEGV, // Stack_Segment_Fault
    [ 0xe  ]  = SIGSEGV, // Page_Fault
};

static uint8_t non_critical_exceptions[] =
{
    [ 0x10 ]  = 1,  // FP_Exception
    [ 0x6  ]  = 1,  // Invalid_Opcode
    [ 0x0  ]  = 1,  // Division_Error
    [ 0x3  ]  = 1,  // Breakpoint_Triggered
    [ 0xc  ]  = 1,  // Stack_Segment_Fault
    [ 0xe  ]  = 1,  // Page_Fault
};

static const char *exceptionstr[] =
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

void x86_outb(uint16_t port, uint8_t val);
void x86_outw(uint16_t port, uint16_t val);
void x86_outsw(unsigned short int __port, const void *__addr,
                                unsigned long int __count);
void x86_insw(unsigned short int __port, void *__addr, unsigned long int __count);
uint8_t x86_inb(uint16_t port);
uint16_t x86_inw(uint16_t port);
void x86_get_msr(uint32_t msr, uint32_t *lo, uint32_t *hi);
void x86_set_msr(uint32_t msr, uint32_t lo, uint32_t hi);
void x86_rdtsc(uint32_t *lo, uint32_t *hi);
uint64_t x86_rdtsc_v();
void x86_cpuid(uint32_t leah, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

#endif