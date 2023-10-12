#pragma once

#include <kernel/kernel.h>
#include <kernel/include/C/typedefs.h>

enum
{
    SVM_SUCCESS = 0,

    SVM_NOT_SUPPORTED = 1,
    SVM_NOT_ENABLED = 2,
    SVM_DISABLED_WITH_KEY = 3,
};

// SVM related structures and stuff from the linux kernel (that'd be so tedious to write them on my own)
// https://github.com/torvalds/linux/blob/master/arch/x86/include/asm/svm.h

enum intercept_words
{
    INTERCEPT_CR = 0,
    INTERCEPT_DR,
    INTERCEPT_EXCEPTION,
    INTERCEPT_WORD3,
    INTERCEPT_WORD4,
    INTERCEPT_WORD5,
    MAX_INTERCEPT,
};

enum
{
    /* Byte offset 000h (word 0) */
    INTERCEPT_CR0_READ = 0,
    INTERCEPT_CR3_READ = 3,
    INTERCEPT_CR4_READ = 4,
    INTERCEPT_CR8_READ = 8,
    INTERCEPT_CR0_WRITE = 16,
    INTERCEPT_CR3_WRITE = 16 + 3,
    INTERCEPT_CR4_WRITE = 16 + 4,
    INTERCEPT_CR8_WRITE = 16 + 8,
    /* Byte offset 004h (word 1) */
    INTERCEPT_DR0_READ = 32,
    INTERCEPT_DR1_READ,
    INTERCEPT_DR2_READ,
    INTERCEPT_DR3_READ,
    INTERCEPT_DR4_READ,
    INTERCEPT_DR5_READ,
    INTERCEPT_DR6_READ,
    INTERCEPT_DR7_READ,
    INTERCEPT_DR0_WRITE = 48,
    INTERCEPT_DR1_WRITE,
    INTERCEPT_DR2_WRITE,
    INTERCEPT_DR3_WRITE,
    INTERCEPT_DR4_WRITE,
    INTERCEPT_DR5_WRITE,
    INTERCEPT_DR6_WRITE,
    INTERCEPT_DR7_WRITE,
    /* Byte offset 008h (word 2) */
    INTERCEPT_EXCEPTION_OFFSET = 64,
    /* Byte offset 00Ch (word 3) */
    INTERCEPT_INTR = 96,
    INTERCEPT_NMI,
    INTERCEPT_SMI,
    INTERCEPT_INIT,
    INTERCEPT_VINTR,
    INTERCEPT_SELECTIVE_CR0,
    INTERCEPT_STORE_IDTR,
    INTERCEPT_STORE_GDTR,
    INTERCEPT_STORE_LDTR,
    INTERCEPT_STORE_TR,
    INTERCEPT_LOAD_IDTR,
    INTERCEPT_LOAD_GDTR,
    INTERCEPT_LOAD_LDTR,
    INTERCEPT_LOAD_TR,
    INTERCEPT_RDTSC,
    INTERCEPT_RDPMC,
    INTERCEPT_PUSHF,
    INTERCEPT_POPF,
    INTERCEPT_CPUID,
    INTERCEPT_RSM,
    INTERCEPT_IRET,
    INTERCEPT_INTn,
    INTERCEPT_INVD,
    INTERCEPT_PAUSE,
    INTERCEPT_HLT,
    INTERCEPT_INVLPG,
    INTERCEPT_INVLPGA,
    INTERCEPT_IOIO_PROT,
    INTERCEPT_MSR_PROT,
    INTERCEPT_TASK_SWITCH,
    INTERCEPT_FERR_FREEZE,
    INTERCEPT_SHUTDOWN,
    /* Byte offset 010h (word 4) */
    INTERCEPT_VMRUN = 128,
    INTERCEPT_VMMCALL,
    INTERCEPT_VMLOAD,
    INTERCEPT_VMSAVE,
    INTERCEPT_STGI,
    INTERCEPT_CLGI,
    INTERCEPT_SKINIT,
    INTERCEPT_RDTSCP,
    INTERCEPT_ICEBP,
    INTERCEPT_WBINVD,
    INTERCEPT_MONITOR,
    INTERCEPT_MWAIT,
    INTERCEPT_MWAIT_COND,
    INTERCEPT_XSETBV,
    INTERCEPT_RDPRU,
    TRAP_EFER_WRITE,
    TRAP_CR0_WRITE,
    TRAP_CR1_WRITE,
    TRAP_CR2_WRITE,
    TRAP_CR3_WRITE,
    TRAP_CR4_WRITE,
    TRAP_CR5_WRITE,
    TRAP_CR6_WRITE,
    TRAP_CR7_WRITE,
    TRAP_CR8_WRITE,
    /* Byte offset 014h (word 5) */
    INTERCEPT_INVLPGB = 160,
    INTERCEPT_INVLPGB_ILLEGAL,
    INTERCEPT_INVPCID,
    INTERCEPT_MCOMMIT,
    INTERCEPT_TLBSYNC,
};

struct __attribute__((packed)) vmcb_control_area
{
    uint32_t intercepts[MAX_INTERCEPT];
    uint32_t reserved_1[15 - MAX_INTERCEPT];
    uint16_t pause_filter_thresh;
    uint16_t pause_filter_count;
    uint64_t iopm_base_pa;
    uint64_t msrpm_base_pa;
    uint64_t tsc_offset;
    uint32_t asid;
    uint8_t tlb_ctl;
    uint8_t reserved_2[3];
    uint32_t int_ctl;
    uint32_t int_vector;
    uint32_t int_state;
    uint8_t reserved_3[4];
    uint32_t exit_code;
    uint32_t exit_code_hi;
    uint64_t exit_info_1;
    uint64_t exit_info_2;
    uint32_t exit_int_info;
    uint32_t exit_int_info_err;
    uint64_t nested_ctl;
    uint64_t avic_vapic_bar;
    uint64_t ghcb_gpa;
    uint32_t event_inj;
    uint32_t event_inj_err;
    uint64_t nested_cr3;
    uint64_t virt_ext;
    uint32_t clean;
    uint32_t reserved_5;
    uint64_t next_rip;
    uint8_t insn_len;
    uint8_t insn_bytes[15];
    uint64_t avic_backing_page; /* Offset 0xe0 */
    uint8_t reserved_6[8];      /* Offset 0xe8 */
    uint64_t avic_logical_id;   /* Offset 0xf0 */
    uint64_t avic_physical_id;  /* Offset 0xf8 */
    uint8_t reserved_7[8];
    uint64_t vmsa_pa; /* Used for an SEV-ES guest */
    uint8_t reserved_8[720];

    /*
     * Offset 0x3e0, 32 bytes reserved
     * for use by hypervisor/software.
     */
    union
    {
        // struct hv_vmcb_enlightenments hv_enlightenments;
        uint8_t reserved_sw[32];
    };
};

struct vmcb_seg
{
    uint16_t selector;
    uint16_t attrib;
    uint32_t limit;
    uint64_t base;
} __attribute__((packed));

struct vmcb_save_area
{
    struct vmcb_seg es;
    struct vmcb_seg cs;
    struct vmcb_seg ss;
    struct vmcb_seg ds;
    struct vmcb_seg fs;
    struct vmcb_seg gs;
    struct vmcb_seg gdtr;
    struct vmcb_seg ldtr;
    struct vmcb_seg idtr;
    struct vmcb_seg tr;
    /* Reserved fields are named following their struct offset */
    uint8_t reserved_0xa0[42];
    uint8_t vmpl;
    uint8_t cpl;
    uint8_t reserved_0xcc[4];
    uint64_t efer;
    uint8_t reserved_0xd8[112];
    uint64_t cr4;
    uint64_t cr3;
    uint64_t cr0;
    uint64_t dr7;
    uint64_t dr6;
    uint64_t rflags;
    uint64_t rip;
    uint8_t reserved_0x180[88];
    uint64_t rsp;
    uint64_t s_cet;
    uint64_t ssp;
    uint64_t isst_addr;
    uint64_t rax;
    uint64_t star;
    uint64_t lstar;
    uint64_t cstar;
    uint64_t sfmask;
    uint64_t kernel_gs_base;
    uint64_t sysenter_cs;
    uint64_t sysenter_esp;
    uint64_t sysenter_eip;
    uint64_t cr2;
    uint8_t reserved_0x248[32];
    uint64_t g_pat;
    uint64_t dbgctl;
    uint64_t br_from;
    uint64_t br_to;
    uint64_t last_excp_from;
    uint64_t last_excp_to;
    uint8_t reserved_0x298[72];
    uint64_t spec_ctrl; /* Guest version of SPEC_CTRL at 0x2E0 */
} __attribute__((packed));

struct vmcb
{
    struct vmcb_control_area control;
    struct vmcb_save_area save;
} __attribute__((packed));

// -----------------------

struct SVM
{
    struct vmcb *__vmcb;
};

int __cpu_svm_init();
int cpu_svm_support();
int cpu_svm_make(struct SVM **svm);

void cpu_svm_cleanup(struct SVM *svm);

static void cpu_svm_vmrun(uintptr_t vmcb)
{
    __asm__ volatile("vmrun %0" ::"r"(vmcb));
}

static void cpu_svm_vmexit(uintptr_t vmcb)
{
    __asm__ volatile("vmexit");
}
