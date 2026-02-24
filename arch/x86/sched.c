#include <kernel/inari.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/proc/sched.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/proc/proc.h>
#include <kernel/sync/spinlock.h>

#include <arch/x86/tss.h>
#include <arch/x86/cpu.h>
#include <arch/paging.h>
#include <misc/string.h>

static spinlock_t x86_sched_lock;

void arch_sched_save(struct thread *task)
{
    uint32_t flags;
    spin_lock_irqsave(&x86_sched_lock, flags);
    struct interrupt_frame *frame = interrupt_frame();
    struct x86_regs32 *regs;
    if (!frame || !task || !task->kernel_stack_pointer)
    {
        spin_unlock_irqrestore(&x86_sched_lock, flags);
        return;
    }
    regs = (struct x86_regs32 *)frame->registers.base;
    task->saved_sp = regs->task_esp;
    spin_unlock_irqrestore(&x86_sched_lock, flags);
}

void arch_sched_load(struct thread *task)
{
    uint32_t flags;
    spin_lock_irqsave(&x86_sched_lock, flags);
    struct interrupt_frame *frame = interrupt_frame();
    struct x86_regs32 *regs;
    uint32_t esp;
    pagedir_t *kernel_pagedir;
    int is_kernel_pagedir;

    if (!frame || !task)
    {
        spin_unlock_irqrestore(&x86_sched_lock, flags);
        return;
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
            esp -= 4; *(((uint32_t*)esp)) = 0x200;
            esp -= 4; *(((uint32_t*)esp)) = 0x1b;
            esp -= 4; *(((uint32_t*)esp)) = (uint32_t)task->entrypoint;
        }

        esp -= 4*2;                           // int_no, err_code
        esp -= 4*8;                           // regs edi-eax

        esp -= 4; *(((uint32_t*)esp)) = is_kernel_pagedir ? 0x10 : 0x23; // ds
        esp -= 4; *(((uint32_t*)esp)) = is_kernel_pagedir ? 0x10 : 0x23; // es
        esp -= 4; *(((uint32_t*)esp)) = is_kernel_pagedir ? 0x10 : 0x23; // fs
        esp -= 4; *(((uint32_t*)esp)) = is_kernel_pagedir ? 0x10 : 0x23; // gs

        task->saved_sp = esp;
    }

    regs->task_cr3 = (uint32_t)arch_virt_to_phys(arch_get_kernel_pagedir(), (void*)task->vmem);
    regs->task_esp = task->saved_sp;

    extern struct tss_entry_struct tss_entry;
    tss_entry.esp0 = (uint32_t)task->kernel_stack_pointer + CONFIG_STACK_SIZE;

    spin_unlock_irqrestore(&x86_sched_lock, flags);
}
