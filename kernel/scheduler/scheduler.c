#include <kernel/kernel.h>
#include <kernel/lock/spinlock.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/scheduler/thread.h>
#include <kernel/include/C/string.h>

#include <drivers/cpu/interrupts/interrupts.h>

// TODO: make atomic
bool scheduler_state = 1;

spinlock_t sched_spinlock;

size_t running_tasks;
int32_t task_idx;

struct scheduler scheds[KERN_MAX_CORES];
struct scheduler_task *tasks;

interrupt_handler_t scheduler_handler(struct cpu_core *core, struct regs32 *regs)
{
    scheduler_switch(core, regs);

    // Execute switched current task
    if (scheds[core->core_id].alive)
        scheduler_execute(core, regs);
}

void scheduler_init()
{
    // Subscribe for the timer IRQs, which will switch scheduler tasks
    cpu_ints_sub(INTERRUPT_TIMER, &scheduler_handler);
    running_tasks = 0;
    task_idx = 0;

    spinlock_init(&sched_spinlock);
}

void scheduler_shutdown()
{
    memset(0, &scheds[0], sizeof(scheds));
    scheduler_state = 0;
}

bool scheduler_alive()
{
    return scheduler_state;
}

void scheduler_switch(struct cpu_core *core, struct regs32 *regs)
{
    struct scheduler *sched = &scheds[core->core_id];

    // Check if we have any tasks running
    if (tasks != NULL)
    {
        if (task_idx >= running_tasks || task_idx < 0)
            task_idx = 0;

        // Save state and release task before switching
        if (sched->current_task != NULL)
        {
            if (sched->flush_regs)
                memcpy(&sched->current_task->regs, regs, sizeof(struct regs32));
            sched->flush_regs = false;
            __sync_lock_release(&sched->current_task->lock.lock);
        }

        sched->current_task = &tasks[task_idx++];
        sched->current_task->owner = sched;
        if (sched->current_task != NULL)
            __sync_lock_release(&sched->current_task->lock.lock);
    }
}

struct scheduler_task *scheduler_current()
{
    return scheds[cpu_current_core()->core_id].current_task;
}

void scheduler_enter(struct cpu_core *core)
{
    scheds[core->core_id].alive = true;
    while (scheds[core->core_id].alive)
    {
        __asm__ volatile("pause":::"memory");
    }
}

void scheduler_execute(struct cpu_core *core, struct regs32 *regs)
{
    struct scheduler *sched = &scheds[core->core_id];
    struct scheduler_task *task = sched->current_task;

    if (task == NULL)
        return;
    if (task->dirty)
        return;

    // try to lock this thread so we can execute it
    if (spinlock_trylock(&task->lock) == 0)
    {
        sched->flush_regs = true;
        regs->edi = task->regs.edi;
        regs->ebp = task->regs.ebp;
        regs->esp = task->regs.esp;
        regs->ebx = task->regs.ebx;
        regs->edx = task->regs.edx;
        regs->ecx = task->regs.ecx;
        regs->eax = task->regs.eax;
        regs->eip = task->regs.eip;
    }
}

void scheduler_task_init(struct scheduler_task *task)
{
    if (task == NULL)
        return;

    memset(0, &task->regs, sizeof(regs32_t));

    // Allocate stack for the thread
    // TODO: do this dynamically
    task->regs.esp = kmalloc(KERN_STACK_SIZE);
    task->regs.eip = task->thread->entry;

    task->dirty = 0;
}

void scheduler_task_cleanup(struct scheduler_task *task)
{
    if (task == NULL)
        return;

    kfree(task->regs.esp);
    task->dirty = 1;
}

int scheduler_append(struct thread *thread)
{
    if (thread == NULL)
        return -1;

    size_t i, task_id;
    int ret = 0;

    spinlock_acquire(&sched_spinlock);
    // Check that we're not running this thread already
    for (i = 0; i < running_tasks; i++)
    {
        if (tasks[i].task_id == thread->tid)
        {
            ret = -1;
            goto end;
        }
    }

    // Resize tasks array and append a new thread there
    tasks = krealloc(tasks, (++running_tasks) * sizeof(struct scheduler_task));
    struct scheduler_task *task = &tasks[(running_tasks - 1)];
    task->dirty = true;
    task->thread = thread;
    task->task_id = (running_tasks - 1);
    spinlock_init(&task->lock);
    scheduler_task_init(task);
    ret = (running_tasks - 1);
end:
    spinlock_release(&sched_spinlock);
    return ret;
}

int scheduler_kill(int32_t taskid)
{
    struct scheduler_task *new_tasks, *found_task;
    int ret = -1, found = 0;
    size_t i;

    spinlock_acquire(&sched_spinlock);
    // Make new list of tasks to be used instead of current one
    new_tasks = kcalloc(running_tasks - 1, sizeof(struct scheduler_task));

    // Check that we're running this task
    for (i = 0; i < running_tasks; i++)
    {
        if (tasks[i].task_id == taskid)
        {
            found_task = &tasks[i];
            found = 1;
        }
        else
        {
            memcpy(&new_tasks[i], &tasks[i], sizeof(struct scheduler_task));
        }
    }

    // We found the task
    if (found)
    {
        scheduler_task_cleanup(found_task);

        ret = 0;
        running_tasks--;
        if (running_tasks == 0)
        {
            // Make the tasks list NULL if we don't have any other tasks
            tasks = NULL;
            kfree(new_tasks);
            goto end;
        }

        kfree(tasks);
        tasks = new_tasks;
    }

end:
    spinlock_release(&sched_spinlock);
    return ret;
}
