#include <kernel/kernel.h>
#include <kernel/core/sched/scheduler.h>
#include <kernel/libc/string.h>

#include <kernel/arch/i686/impl.h>

extern void scheduler_task_entrypoint();

struct i686_task_regs
{
    uint32_t esp;
    uint32_t ss;
};

void arch_scheduler_core_idle()
{
    __enable_int();
    __halt();
}

void arch_scheduler_load_current_task(struct sched_core *core, void *regs_ptr)
{
    struct i686_task_regs *arch_regs = (struct i686_task_regs*)&core->current_task->regs[0];

    #ifdef SCHEDULER_DEBUG
        struct regs32 *regs = (struct regs32 *)regs_ptr;
        printk("sched: switching to task tid=%d esp=%p -> %p",
            core->current_task->tid,
            arch_regs->esp, regs->esp);
    #endif

    // Perform complete context switch to the task
    __asm__ volatile(
        "mov %0, %%esp\n"
        "popa\n"
        "add $8, %%esp\n"
        "sti\n"
        "iret\n"
        :
        : "m"(arch_regs->esp)
        : "memory"
    );
}

void arch_scheduler_save_current_task(struct sched_core *core, void *regs_ptr)
{
    struct i686_task_regs *arch_regs = (struct i686_task_regs*)&core->current_task->regs[0];
    struct regs32 *regs = (struct regs32 *)regs_ptr;

    #ifdef SCHEDULER_DEBUG
        printk("sched: saving current task tid=%d esp=%p -> %p",
            core->current_task->tid,
            arch_regs->esp, regs->esp);
    #endif

    arch_regs->ss = 0x10; // regs->ss;
    arch_regs->esp = regs->esp_pushad; // In idt.asm we push 2 dwords, so we need to add 8
}

void arch_scheduler_init_task(struct sched_task *task)
{
    struct i686_task_regs *arch_regs = (struct i686_task_regs*)&task->regs[0];

    // Initialize all registers
    arch_regs->esp = ((unsigned int)task->stack_base + KERN_STACK_SIZE) & (~0xF);
    arch_regs->ss = 0x10;

    // Fill the stack with registers that will be later used
    uint32_t *stack_array = (uint32_t *)arch_regs->esp;
    uint32_t offset = 1;

    *(stack_array - (offset++)) = 0x202;            // set eflags value
    *(stack_array - (offset++)) = 0x08;             // set CS value
    *(stack_array - (offset++)) = (unsigned int)&scheduler_task_entrypoint; // set EIP value

    *(stack_array - (offset++)) = 0;
    *(stack_array - (offset++)) = 0;

    *(stack_array - (offset++)) = 0;                // EAX
    *(stack_array - (offset++)) = 0;                // ECX
    *(stack_array - (offset++)) = 0;                // EDX
    *(stack_array - (offset++)) = 0;                // EBX
    *(stack_array - (offset++)) = arch_regs->esp;   // ESP
    *(stack_array - (offset++)) = arch_regs->esp;   // EBP
    *(stack_array - (offset++)) = 0;                // ESI
    *(stack_array - (offset++)) = 0;                // EDI

    arch_regs->esp -= (offset - 1) * 4;
}
