#include <kernel/inari.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/proc/sched.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/errno.h>
#include <kernel/proc/proc.h>
#include <kernel/sync/spinlock.h>

#include <arch/x86/tss.h>
#include <arch/x86/cpu.h>
#include <arch/x86/arch.h>
#include <arch/paging.h>
#include <misc/string.h>

static spinlock_t x86_sched_lock;

void arch_sched_save(struct thread *task)
{
    uint32_t flags;
    spin_lock_irqsave(&x86_sched_lock, flags);
    struct interrupt_frame *frame = interrupt_get_frame();
    struct x86_regs32 *regs;
    if (!frame || !task || !task->kernel_stack_pointer)
    {
        spin_unlock_irqrestore(&x86_sched_lock, flags);
        return;
    }
    regs = (struct x86_regs32 *)frame->registers.base;
    if ((regs->cs & 0x03) == 0x03) /* Came from ring3 */
        task->saved_stack = (void*)regs->task_esp;
    else /* Came from ring0 */
        task->saved_stack = (void*)((struct x86_regs32_ring0*)regs)->task_esp;
    spin_unlock_irqrestore(&x86_sched_lock, flags);
}

int arch_sched_load(struct thread *task)
{
    uint32_t flags;
    spin_lock_irqsave(&x86_sched_lock, flags);
    struct interrupt_frame *frame = interrupt_get_frame();
    struct x86_regs32 *regs;
    struct x86_regs32 *regs_frame = (struct x86_regs32 *)(task->saved_stack ? (task->saved_stack - 8) : NULL);
    uint32_t esp;
    pagedir_t *kernel_pagedir;
    int is_kernel_pagedir;

    if (!frame || !task)
    {
        spin_unlock_irqrestore(&x86_sched_lock, flags);
        return -1;
    }
    kernel_pagedir = arch_get_kernel_pagedir();
    is_kernel_pagedir = task->vmem == kernel_pagedir;
    regs = (struct x86_regs32 *)frame->registers.base;
    
    /* If the stack pointer is not initialized, that's the first time this task is scheduled */
    if (!task->kernel_stack_pointer)
    {
        task->kernel_stack_pointer = vmm_alloc_kernel((CONFIG_STACK_SIZE >> 12) + 1);
        if (task->kernel_stack_pointer == NULL)
            panic("sched: OOM when allocating kernel stack.");

        esp = (uint32_t)task->kernel_stack_pointer + (uint32_t)CONFIG_STACK_SIZE;
        
        /* Stack for kernel and userspace tasks is different */
        if (is_kernel_pagedir)
        {
            esp -= 4; *(((uint32_t*)esp)) = ((struct x86_regs32*)frame->registers.base)->eflags;
            esp -= 4; *(((uint32_t*)esp)) = 0x08;
            esp -= 4; *(((uint32_t*)esp)) = (uint32_t)&sched_thread_preentry;
        }
        else {
            task->thread_stack_pointer = vmm_alloc_user(task->vmem, (CONFIG_STACK_SIZE >> 12) + 1);
            if (task->thread_stack_pointer == NULL)
                panic("sched: OOM when allocating user stack.");

            esp -= 4; *(((uint32_t*)esp)) = 0x23; // ss
            esp -= 4; *(((uint32_t*)esp)) = (uint32_t)task->thread_stack_pointer + (uint32_t)CONFIG_STACK_SIZE;   // useresp
            esp -= 4; *(((uint32_t*)esp)) = 0x202;
            esp -= 4; *(((uint32_t*)esp)) = 0x1b;
            esp -= 4; *(((uint32_t*)esp)) = (uint32_t)task->entrypoint;
        }

        esp -= 4*2;                           // int_no, err_code
        esp -= 4*8;                           // regs edi-eax
        memset((void*)esp, 0, 40);

        uint32_t segments = is_kernel_pagedir ? 0x10 : 0x23;
        esp -= 4; *(((uint32_t*)esp)) = segments; // ds
        esp -= 4; *(((uint32_t*)esp)) = segments; // es
        esp -= 4; *(((uint32_t*)esp)) = segments; // fs
        esp -= 4; *(((uint32_t*)esp)) = segments; // gs

        task->saved_stack = (void*)esp;
    }
    else if (task->proc_data && !is_kernel_pagedir && task->proc_data->pending_signal != 0 && (regs_frame->cs & 0x03) == 0x03)
    {
        /* Push onto stack the signal frame */
        if (task->proc_data->signal_handler[task->proc_data->pending_signal] && !task->sig_saved_stack)
        {
            /* Hijack thread execution */
            arch_switch_pagedir(task->vmem);
            task->sig_saved_stack = kmalloc(sizeof(struct x86_regs32));
            task->sig_saved_stack_base = task->saved_stack;
            memcpy(task->sig_saved_stack, task->saved_stack, sizeof(struct x86_regs32) - 8);
            volatile uint32_t test = (uint32_t)&regs_frame->useresp;
            regs_frame->useresp -= 4;
            *((uint32_t*)regs_frame->useresp) = 0x0;
            regs_frame->useresp -= 4;
            *((uint32_t*)regs_frame->useresp) = task->proc_data->pending_signal;
            regs_frame->useresp -= 4;
            *((uint32_t*)regs_frame->useresp) = 0x0;
            regs_frame->eip = (uint32_t)task->proc_data->signal_handler[task->proc_data->pending_signal];
            arch_switch_pagedir(kernel_pagedir);

            task->proc_data->pending_signal = 0;
        }
        else if (non_critical_exceptions[task->proc_data->pending_signal] == 0) {
            /* Kill process if critical signal and no handler */
            kill_process(task->proc_data->pid, -EKILLED);
            spin_unlock_irqrestore(&x86_sched_lock, flags);
            return -1;
        }
    }

    regs->task_cr3 = (uint32_t)arch_virt_to_phys(arch_get_kernel_pagedir(), (void*)task->vmem);
    regs->task_esp = (uint32_t)task->saved_stack;

    extern struct tss_entry_struct tss_entry;
    tss_entry.esp0 = (uint32_t)task->kernel_stack_pointer + CONFIG_STACK_SIZE;

    spin_unlock_irqrestore(&x86_sched_lock, flags);
    return 0;
}

void arch_sched_sigreturn(struct thread *task)
{
    uint32_t flags;
    spin_lock_irqsave(&x86_sched_lock, flags);

    /* Signal handler pushed onto stack */
    if (task->sig_saved_stack)
    {
        task->saved_stack = task->sig_saved_stack_base;
        memcpy((void*)task->saved_stack, (void*)task->sig_saved_stack, sizeof(struct x86_regs32) - 8);
        kfree(task->sig_saved_stack);
        task->sig_saved_stack_base = NULL;
        task->sig_saved_stack = NULL;
        task->flags |= SCHED_FLAG_SIGRETURN;
    }

    spin_unlock_irqrestore(&x86_sched_lock, flags);
}