// Dumb implementation of a scheduler, but it should work for now...

#include <kernel/kernel.h>
#include <kernel/lock/spinlock.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/scheduler/thread.h>
#include <kernel/include/C/string.h>

#include <drivers/cpu/interrupts/interrupts.h>

bool scheduler_state = 1;

spinlock_t sched_spinlock;

size_t running_tasks;

struct scheduler scheds[KERN_MAX_CORES];
struct scheduler_task *tasks;

interrupt_handler_t scheduler_handler(struct cpu_core *core, struct regs32 *regs)
{
    // Switch task if the core is alive and 4 ticks passed since last switch
    if (scheds[core->core_id].alive && scheds[core->core_id].counter++ % 4 == 0)
    {
        scheduler_switch(core, regs);
    }
}

void scheduler_init()
{
    printk("scheduler: initializing");
    running_tasks = 0;

    memset((void*)&scheds[0], 0, sizeof(scheds));
    spinlock_init(&sched_spinlock);
}

void scheduler_shutdown()
{
    memset((void*)&scheds[0], 0, sizeof(scheds));
    scheduler_state = 0;
}

bool scheduler_alive()
{
    return scheduler_state;
}

void scheduler_switch(struct cpu_core *core, struct regs32 *regs)
{
    spinlock_acquire(&sched_spinlock);
    size_t retries;
    struct scheduler *sched = &scheds[core->core_id];
    struct scheduler_task *next_task = NULL;

    // Check if we have any tasks running
    if (tasks != NULL)
    {
        // Save state and release task before switching
        if (sched->current_task != NULL) // && sched->current_task->lock.lock)
        {
            sched->current_task->owner = NULL;
            sched->current_task->regs.edi = regs->edi;
            sched->current_task->regs.ebp = regs->ebp;
            sched->current_task->regs.esp = regs->useresp;
            sched->current_task->regs.ebx = regs->ebx;
            sched->current_task->regs.edx = regs->edx;
            sched->current_task->regs.ecx = regs->ecx;
            sched->current_task->regs.eax = regs->eax;
            sched->current_task->regs.eip = regs->eip;
            sched->current_task->regs.ds = regs->ds;
            sched->current_task->regs.es = regs->es;
            sched->current_task->regs.fs = regs->fs;
            sched->current_task->regs.gs = regs->gs;
            __sync_lock_release(&sched->current_task->lock.lock);
        }

        // Select next task to execute
        sched->current_task = NULL;
        do
        {
            // Iterate thru all available tasks and find the one thats not being executed right now
            next_task = NULL;
            while (next_task == NULL)
            {
                if (sched->last_task_idx >= running_tasks || sched->last_task_idx < 0)
                    sched->last_task_idx = 0;

                next_task = &tasks[sched->last_task_idx++];
            }

            // Try to lock the task, so we would not interrupt it if it's being executed
            if (spinlock_trylock(&next_task->lock) == 0) {
                if (next_task->thread->state == THREAD_ACTIVE && !next_task->owner)
                {
                    break;
                }
                else if (next_task->owner == sched)
                {
                    // FIXME: we should not really panic here. I did it just to notice if it ever happens
                    panic("scheduler: current core is task owner, but the core does not execute it; task_id=%d", next_task->task_id);
                }

                // Unlock the task if didn't choice it as the next one
                __sync_lock_release(&sched->current_task->lock.lock);
            }
        } while (retries++ < 128);

        // If we were unable to find next task to execute in 128 retries, idle the core
        if (next_task != NULL)
        {
            sched->current_task = next_task;
            sched->current_task->owner = sched;

            regs->edi = sched->current_task->regs.edi;
            regs->ebp = sched->current_task->regs.ebp;
            regs->useresp = sched->current_task->regs.esp;
            regs->ebx = sched->current_task->regs.ebx;
            regs->edx = sched->current_task->regs.edx;
            regs->ecx = sched->current_task->regs.ecx;
            regs->eax = sched->current_task->regs.eax;
            regs->eip = sched->current_task->regs.eip;
            regs->ds = sched->current_task->regs.ds;
            regs->es = sched->current_task->regs.es;
            regs->fs = sched->current_task->regs.fs;
            regs->gs = sched->current_task->regs.gs;
            goto end;
        }
    }
    regs->eip = (uint32_t)&scheduler_core_idle;

end:
    spinlock_release(&sched_spinlock);
}

struct scheduler_task *scheduler_current()
{
    return scheds[cpu_current_core()->core_id].current_task;
}

// here the core will be idling if no tasks are available
__attribute__((optimize("O0"))) void scheduler_core_idle()
{
    do {} while (1);
}

// Initializes and starts the scheduler for the current core, and then enters an infinite loop.
// When the core would receive an interrupt that makes it to start executing scheduler tasks,
// it will never return to this function, and if no tasks available, will be idled at the "scheduler_core_idle"
void scheduler_enter(struct cpu_core *core)
{
    printk("sched: CPU[%d] is now running", core->core_id);
    scheds[core->core_id].alive = true;
    while (scheds[core->core_id].alive)
    {
        __asm__ volatile("pause" ::: "memory");
    }
}

void scheduler_task_init(struct scheduler_task *task)
{
    if (task == NULL)
        return;

    memset((void*)&task->regs, 0, sizeof(regs32_t));

    // Allocate stack for the thread
    // TODO: do this dynamically
    task->regs.esp = (uint32_t)kmalloc(KERN_STACK_SIZE);
    task->regs.eip = (uint32_t)&thread_entrypoint;
    task->regs.ds = 0x10;
    task->regs.es = 0x10;
    task->regs.fs = 0x10;
    task->regs.gs = 0x10;
}

void scheduler_task_cleanup(struct scheduler_task *task)
{
    if (task == NULL)
        return;

    kfree((void*)task->regs.esp);
}

int scheduler_append(struct thread *thread)
{
    if (thread == NULL)
        return 1;

    size_t i, task_id;
    int ret = 0;

    spinlock_acquire(&sched_spinlock);
    // Check that we're not running this thread already
    for (i = 0; i < running_tasks; i++)
    {
        if (tasks[i].task_id == thread->tid)
        {
            ret = 2;
            goto end;
        }
    }

    // Resize tasks array and append a new thread there
    task_id = running_tasks++;
    ret = task_id;

    tasks = krealloc(tasks, (task_id + 1) * sizeof(struct scheduler_task));
    struct scheduler_task *task = &tasks[task_id];
    task->thread = thread;
    task->task_id = task_id;
    task->owner = NULL;
    spinlock_init(&task->lock);
    scheduler_task_init(task);
end:
    spinlock_release(&sched_spinlock);
    return ret;
}

int scheduler_restart(int32_t taskid)
{
    int ret = 1, i = 0;
    struct scheduler_task *task;
    spinlock_acquire(&sched_spinlock);

    // Check that we actually have task with such id
    if (running_tasks == 0) {
        // No tasks are running
        goto end;
    }

    do {
        task = &tasks[i++];
    } while (task->task_id != taskid);

    if (task == NULL || task->task_id != taskid) {
        // Task not found
        ret = 2;
        goto end;
    }

    // Just clean up the task and initialize again
    scheduler_task_cleanup(task);
    scheduler_task_init(task);
    spinlock_release(&task->lock);
    task->owner = NULL;
    ret = 0;
end:
    spinlock_release(&sched_spinlock);
    return ret;
}

int scheduler_kill(int32_t taskid)
{
    struct scheduler_task *new_tasks, *found_task;
    int ret = 1, found = 0;
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
