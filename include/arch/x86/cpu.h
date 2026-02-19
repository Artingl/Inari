#ifndef _INARI_X86_CPU
#define _INARI_X86_CPU

#include <misc/types.h>

enum {
    CPU_FEATURE_ECX_SSE3         = 1 << 0, 
    CPU_FEATURE_ECX_PCLMUL       = 1 << 1,
    CPU_FEATURE_ECX_DTES64       = 1 << 2,
    CPU_FEATURE_ECX_MONITOR      = 1 << 3,  
    CPU_FEATURE_ECX_DS_CPL       = 1 << 4,  
    CPU_FEATURE_ECX_VMX          = 1 << 5,  
    CPU_FEATURE_ECX_SMX          = 1 << 6,  
    CPU_FEATURE_ECX_EST          = 1 << 7,  
    CPU_FEATURE_ECX_TM2          = 1 << 8,  
    CPU_FEATURE_ECX_SSSE3        = 1 << 9,  
    CPU_FEATURE_ECX_CID          = 1 << 10,
    CPU_FEATURE_ECX_SDBG         = 1 << 11,
    CPU_FEATURE_ECX_FMA          = 1 << 12,
    CPU_FEATURE_ECX_CX16         = 1 << 13, 
    CPU_FEATURE_ECX_XTPR         = 1 << 14, 
    CPU_FEATURE_ECX_PDCM         = 1 << 15, 
    CPU_FEATURE_ECX_PCID         = 1 << 17, 
    CPU_FEATURE_ECX_DCA          = 1 << 18, 
    CPU_FEATURE_ECX_SSE4_1       = 1 << 19, 
    CPU_FEATURE_ECX_SSE4_2       = 1 << 20, 
    CPU_FEATURE_ECX_X2APIC       = 1 << 21, 
    CPU_FEATURE_ECX_MOVBE        = 1 << 22, 
    CPU_FEATURE_ECX_POPCNT       = 1 << 23, 
    CPU_FEATURE_ECX_TSC          = 1 << 24, 
    CPU_FEATURE_ECX_AES          = 1 << 25, 
    CPU_FEATURE_ECX_XSAVE        = 1 << 26, 
    CPU_FEATURE_ECX_OSXSAVE      = 1 << 27, 
    CPU_FEATURE_ECX_AVX          = 1 << 28,
    CPU_FEATURE_ECX_F16C         = 1 << 29,
    CPU_FEATURE_ECX_RDRAND       = 1 << 30,
    CPU_FEATURE_ECX_HYPERVISOR   = 1 << 31,
 
    CPU_FEATURE_EDX_FPU          = 1 << 0,  
    CPU_FEATURE_EDX_VME          = 1 << 1,  
    CPU_FEATURE_EDX_DE           = 1 << 2,  
    CPU_FEATURE_EDX_PSE          = 1 << 3,  
    CPU_FEATURE_EDX_TSC          = 1 << 4,  
    CPU_FEATURE_EDX_MSR          = 1 << 5,  
    CPU_FEATURE_EDX_PAE          = 1 << 6,  
    CPU_FEATURE_EDX_MCE          = 1 << 7,  
    CPU_FEATURE_EDX_CX8          = 1 << 8,  
    CPU_FEATURE_EDX_APIC         = 1 << 9,  
    CPU_FEATURE_EDX_SEP          = 1 << 11, 
    CPU_FEATURE_EDX_MTRR         = 1 << 12, 
    CPU_FEATURE_EDX_PGE          = 1 << 13, 
    CPU_FEATURE_EDX_MCA          = 1 << 14, 
    CPU_FEATURE_EDX_CMOV         = 1 << 15, 
    CPU_FEATURE_EDX_PAT          = 1 << 16, 
    CPU_FEATURE_EDX_PSE36        = 1 << 17, 
    CPU_FEATURE_EDX_PSN          = 1 << 18, 
    CPU_FEATURE_EDX_CLFLUSH      = 1 << 19, 
    CPU_FEATURE_EDX_DS           = 1 << 21, 
    CPU_FEATURE_EDX_ACPI         = 1 << 22, 
    CPU_FEATURE_EDX_MMX          = 1 << 23, 
    CPU_FEATURE_EDX_FXSR         = 1 << 24, 
    CPU_FEATURE_EDX_SSE          = 1 << 25, 
    CPU_FEATURE_EDX_SSE2         = 1 << 26, 
    CPU_FEATURE_EDX_SS           = 1 << 27, 
    CPU_FEATURE_EDX_HTT          = 1 << 28, 
    CPU_FEATURE_EDX_TM           = 1 << 29, 
    CPU_FEATURE_EDX_IA64         = 1 << 30,
    CPU_FEATURE_EDX_PBE          = 1 << 31
};

struct x86_regs32
{
    uint32_t task_cr3, task_esp;

    uint32_t gs, fs, es, ds;//, cr3;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed));

struct x86_cpu_idt_descriptor
{
    uint16_t size;
    uint32_t base;
} __attribute__((packed));

struct x86_cpu_idt
{
    unsigned short base_low;
    unsigned short sel;
    unsigned char zero;
    unsigned char flags;
    unsigned short base_high;
} __attribute__((packed));

struct x86_cpu
{
    uint8_t is_available;
    uint32_t core_id;

    uint8_t lapic_id;
    uint32_t lapic_ptr;

    struct x86_cpu_idt_descriptor idt_desc;
    struct x86_cpu_idt *idt;
};

extern int x86_cpu_init(void);
extern const char *x86_cpu_vendor(void);
extern const char *x86_cpu_model(void);
extern uint32_t x86_cpu_features_ecx(void);
extern uint32_t x86_cpu_features_edx(void);
extern void x86_cpu_ack_irq(uint8_t irq);

static inline void x86_cpu_install_idt(uint32_t core_id, uintptr_t base, uint8_t num, uint16_t sel, uint8_t flags)
{
    extern struct x86_cpu x86_cpus_pool[];
    if (core_id >= CONFIG_MAX_CORES || !x86_cpus_pool[core_id].is_available)
    {
        printk("arch: invalid IDT core_id");
        return;
    }

    x86_cpus_pool[core_id].idt[num].base_low = ((uint64_t)base & 0xFFFF);
    x86_cpus_pool[core_id].idt[num].base_high = ((uint64_t)base >> 16) & 0xFFFF;
    x86_cpus_pool[core_id].idt[num].sel = sel;
    x86_cpus_pool[core_id].idt[num].zero = 0;
    x86_cpus_pool[core_id].idt[num].flags = flags | 0x60;
}

#endif