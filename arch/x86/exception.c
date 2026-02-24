#include <kernel/inari.h>
#include <kernel/proc/sched.h>
#include <kernel/proc/proc.h>
#include <kernel/interrupts/swi.h>
#include <kernel/proc/signals.h>
#include <kernel/console/console.h>
#include <kernel/proc/elf.h>

#include <multiboot/multiboot.h>

#include <misc/print.h>
#include <stdarg.h>

#include <arch/x86/cpu.h>
#include <arch/x86/arch.h>
#include <arch/x86/exception.h>

#define DECL_EXCP(n) extern void _arch_excp##n(void);x86_cpu_install_idt(core->core_id, (unsigned)_arch_excp##n, n, 0x08, IDT_PRESENT | IDT_INT32_GATE)

struct stackframe {
    struct stackframe* ebp;
    uint32_t eip;
};

struct kernel_symbols {
    struct elf32_sym* symtab;
    char* strtab;
    uint32_t sym_count;
};

#define SHT_SYMTAB      2
#define SHT_STRTAB      3
#define STT_FUNC        2
#define ELF32_ST_TYPE(INFO) ((INFO) & 0x0F)

static int in_exception = 0;
static struct kernel_symbols ksyms = {0};

static inline void do_printf_handler(char c, void*)
{
    console_printc(CONSOLE_PRINTK, &c, 1);
}

static inline void helper_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    do_printkn(fmt, args, &do_printf_handler, NULL);
    va_end(args);
}

static const char *retrieve_symbol(uintptr_t base)
{
    size_t i;
    char *sym = "???";
    bootinfo_t info = get_boot_info();
    multiboot_info_t *multiboot = (multiboot_info_t*)info.bootloader_info;
    struct elf32_shdr* shdr = (struct elf32_shdr*)multiboot->u.elf_sec.addr;
    /* TODO: Must be bootloader agnostic to access symbols */

    /* Find the .symtab and .strtab if present */
    if (multiboot->flags & MULTIBOOT_INFO_ELF_SHDR && !ksyms.strtab)
    {
        for (i = 0; i < multiboot->u.elf_sec.num; i++)
        {
            if (shdr[i].sh_type == SHT_SYMTAB)
            {
                ksyms.symtab = (struct elf32_sym*)(shdr[i].sh_addr);
                ksyms.sym_count = shdr[i].sh_size / shdr[i].sh_entsize;
                
                uint32_t strtab_idx = shdr[i].sh_link;
                if (strtab_idx < multiboot->u.elf_sec.num) {
                    ksyms.strtab = (char*)(shdr[strtab_idx].sh_addr);
                }
                break;
            }
        }
    }
    
    if (ksyms.symtab != NULL && ksyms.strtab != NULL) {
        for (i = 0; i < ksyms.sym_count; i++)
        {
            if (ELF32_ST_TYPE(ksyms.symtab[i].st_info) != STT_FUNC) continue;

            if (base >= ksyms.symtab[i].st_value && base < (ksyms.symtab[i].st_value + ksyms.symtab[i].st_size))
            {
                sym = (char*)(ksyms.strtab + ksyms.symtab[i].st_name);
                break;
            }

        }
    }

    char *dup = sym;
    if (!*dup) sym = "???";
    while (*dup++)
        if ((*dup > 0x7e || *dup <= 0x20) && *dup != 0)
        {
            sym = "???";
            break;
        }

    return sym;
}

void _print_stacktrace(void *print_handler)
{
    uintptr_t i;
    struct stackframe *stk;
    char *ksym_name;

    __asm__ volatile("movl %%ebp,%0" : "=r"(stk) ::);
    ((void(*)(const char*, ...))print_handler)("Stack trace:\n");
    for (i = 0; stk && (uint32_t)stk >= 0xC0000000 && i < 32; i++)
    {
        ksym_name = retrieve_symbol(stk->eip);
        ((void(*)(const char*, ...))print_handler)("  %s @ 0x%x\n", ksym_name, stk->eip);
        stk = stk->ebp;
    }
}

void x86_exception_setup(struct x86_cpu *core)
{
    DECL_EXCP(0);
    DECL_EXCP(1);
    DECL_EXCP(2);
    DECL_EXCP(3);
    DECL_EXCP(4);
    DECL_EXCP(5);
    DECL_EXCP(6);
    DECL_EXCP(7);
    DECL_EXCP(8);
    DECL_EXCP(9);
    DECL_EXCP(10);
    DECL_EXCP(11);
    DECL_EXCP(12);
    DECL_EXCP(13);
    DECL_EXCP(14);
    DECL_EXCP(16);
    DECL_EXCP(17);
    DECL_EXCP(18);
    DECL_EXCP(19);
    DECL_EXCP(20);
    DECL_EXCP(21);
    DECL_EXCP(28);
    DECL_EXCP(29);
    DECL_EXCP(30);
}

void x86_exception_handler(struct x86_regs32 *regs)
{
    arch_switch_pagedir(arch_get_kernel_pagedir());
    console_switch_early();
    if (in_exception)
        panic("Nested exception");

    in_exception = 1;
    tid_t tid;
    struct thread *th;
    uint8_t is_critical = non_critical_exceptions[regs->int_no] != 1;

    if (sched_current_thread(&tid) == 0 && sched_get_thread(tid, &th) == 0)
    {
        /* If not critical exception, try to call handler */
        if (!is_critical) {
            if (th->proc_data)
                is_critical = proc_signal(th->proc_data->pid, signal_exception[regs->int_no]) != 0;
            else is_critical = 1;
        }

        /* If critical OR the handler above failed OR standalone thread (not process), kill process/thread */
        if (is_critical)
        {
#ifdef CONFIG_DEBUG
            if (regs->int_no == 0xe)
            {
                uint32_t cr2;
                __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
                printk("arch: Page Fault caused by accessing address: 0x%x\n", cr2);
            }
            printk("arch: Exception %s; eip=0x%x (%s); esp=0x%x", exceptionstr[regs->int_no], regs->eip, retrieve_symbol(regs->eip), regs->esp);
            _print_stacktrace(&helper_printf);
#endif
            
            if (th->proc_data)
                kill_process(th->proc_data->pid, -1);
            else
                sched_kill_thread(tid);
        }
    }
    else {
        /* Exception in kernel code */
        panic("kernel exception %s", exceptionstr[regs->int_no]);
    }

    /* Use interrupt dispatcher to reschedule */
    interrupt_dispatch((struct interrupt_frame){
        .int_no = SWI_RESCHEDULE,
        .registers = {
            .base = regs,
            .size = sizeof(struct x86_regs32)
        }
    });

    console_switch_normal();
    in_exception = 0;
}

