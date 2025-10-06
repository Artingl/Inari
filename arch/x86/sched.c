#include <kernel/inari.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/sched/sched.h>
#include <kernel/mm/kmalloc.h>

#include <arch/x86/cpu.h>
#include <misc/string.h>

void arch_sched_save(struct sched_task *task)
{
    struct interrupt_frame *frame = interrupt_frame();
    struct x86_regs32 *regs;
    if (!frame || !task || !task->stack_pointer)
        return;
    regs = (struct x86_regs32 *)frame->registers.base;
    task->saved_sp = regs->task_esp;
}

void arch_sched_load(struct sched_task *task)
{
    struct interrupt_frame *frame = interrupt_frame();
    struct x86_regs32 *regs;
    uint32_t esp;

    if (!frame || !task)
        return;
    regs = (struct x86_regs32 *)frame->registers.base;
    
    if (!task->stack_pointer)
    {
        /* If the stack pointer is not initialized, that's the first time this task is scheduled */
        task->stack_pointer = kmalloc(CONFIG_STACK_SIZE + PAGE_SIZE);

        esp = (uint32_t)task->stack_pointer + (uint32_t)CONFIG_STACK_SIZE;
        esp -= 4; *(((uint32_t*)esp)) = 0x10;   // ss
        esp -= 4; *(((uint32_t*)esp)) = (uint32_t)task->stack_pointer + (uint32_t)CONFIG_STACK_SIZE;   // useresp
        esp -= 4; *(((uint32_t*)esp)) = ((struct x86_regs32*)frame->registers.base)->eflags;
        esp -= 4; *(((uint32_t*)esp)) = ((struct x86_regs32*)frame->registers.base)->cs;
        esp -= 4; *(((uint32_t*)esp)) = (uint32_t)&sched_task_preentry;

        esp -= 4*2;                           // int_no, err_code
        esp -= 4*8;                           // regs edi-eax

        esp -= 4; *(((uint32_t*)esp)) = 0x10; // ds
        esp -= 4; *(((uint32_t*)esp)) = 0x10; // es
        esp -= 4; *(((uint32_t*)esp)) = 0x10; // fs
        esp -= 4; *(((uint32_t*)esp)) = 0x10; // gs

        task->saved_sp = esp;
    }

    regs->task_esp = task->saved_sp;
}
