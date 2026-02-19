#include <kernel/inari.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/proc/sched.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/page.h>
#include <kernel/proc/proc.h>
#include <kernel/sync/spinlock.h>

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
    if (!frame || !task || !task->stack_pointer)
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
    pagedir_t prev_dir = NULL;
    uint32_t esp;

    if (!frame || !task)
    {
        spin_unlock_irqrestore(&x86_sched_lock, flags);
        return;
    }
    regs = (struct x86_regs32 *)frame->registers.base;
    prev_dir = page_get_dir();
    page_switch_dir(task->vmem);
    
    if (!task->stack_pointer)
    {
        /* If the stack pointer is not initialized, that's the first time this task is scheduled */
        task->stack_pointer = kmalloc(CONFIG_STACK_SIZE + PAGE_SIZE);
        if (task->stack_pointer == NULL)
            panic("sched: no memory left.");

        esp = (uint32_t)task->stack_pointer + (uint32_t)CONFIG_STACK_SIZE;
        esp -= 4; *(((uint32_t*)esp)) = 0x10;   // ss
        esp -= 4; *(((uint32_t*)esp)) = (uint32_t)task->stack_pointer + (uint32_t)CONFIG_STACK_SIZE;   // useresp
        esp -= 4; *(((uint32_t*)esp)) = ((struct x86_regs32*)frame->registers.base)->eflags;
        esp -= 4; *(((uint32_t*)esp)) = ((struct x86_regs32*)frame->registers.base)->cs;
        esp -= 4; *(((uint32_t*)esp)) = (uint32_t)&sched_thread_preentry;

        esp -= 4*2;                           // int_no, err_code
        esp -= 4*8;                           // regs edi-eax

        // esp -= 4; *(((uint32_t*)esp)) = (uint32_t)arch_virt_to_phys((void*)task->vmem); // cr3

        esp -= 4; *(((uint32_t*)esp)) = 0x10; // ds
        esp -= 4; *(((uint32_t*)esp)) = 0x10; // es
        esp -= 4; *(((uint32_t*)esp)) = 0x10; // fs
        esp -= 4; *(((uint32_t*)esp)) = 0x10; // gs

        task->saved_sp = esp;
    }
    /* Check if syscall needs to overwrite register values */
    else if (task->flags & SCHED_FLAG_SYSCALL_RSLT)
    {
        task->flags &= ~SCHED_FLAG_SYSCALL_RSLT;
        // ((struct x86_regs32*)task->saved_sp)->ebx = task->syscall_result;
        printk("caught: 0x%x", task->saved_sp);
    }

    regs->task_cr3 = (uint32_t)arch_virt_to_phys((void*)task->vmem);
    regs->task_esp = task->saved_sp;

    page_switch_dir(prev_dir);
    spin_unlock_irqrestore(&x86_sched_lock, flags);
}
