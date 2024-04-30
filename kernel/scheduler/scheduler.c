#include <kernel/kernel.h>
#include <kernel/lock/spinlock.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/scheduler/thread.h>
#include <kernel/include/C/string.h>

#include <drivers/cpu/interrupts/interrupts.h>

extern void scheduler_sleep();

// TODO: make atomic
bool scheduler_state = 1;

spinlock_t sched_spinlock;

size_t running_tasks;
int32_t task_idx;

struct scheduler scheds[KERN_MAX_CORES];
struct scheduler_task *tasks;

interrupt_handler_t scheduler_handler(struct cpu_core *core, struct regs32 *regs)
{
    if (scheds[core->core_id].alive)
    {
        scheduler_switch(core, regs);
    }
}

void scheduler_init()
{
    printk("scheduler: initializing");
    running_tasks = 0;
    task_idx = 0;

    // memset(0, &scheds[0], sizeof(scheds));
    // spinlock_init(&sched_spinlock);
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
    if (!core->is_bsp)
    {
        regs->eip = (uint32_t)&scheduler_sleep;
        return;
    }

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
                if (task_idx >= running_tasks || task_idx < 0)
                    task_idx = 0;

                next_task = &tasks[task_idx++];
            }

            if (!next_task->owner || next_task->owner == sched)
            {
                if (spinlock_trylock(&next_task->lock) == 0)
                    break;
            }
        } while (1);

        sched->current_task = next_task;
        sched->current_task->owner = sched;

typedef struct context {
    uint32_t eax; // 0
    uint32_t ecx; // 4
    uint32_t edx; // 8
    uint32_t ebx; // 12
    uint32_t esp; // 16
    uint32_t ebp; // 20
    uint32_t esi; // 24
    uint32_t edi; // 28
    uint32_t eflags; //32
    uint32_t cr3; // 36
    uint32_t eip; //40
} context_t;

        extern void scheduler_regs_switch(context_t *r);

        context_t t = {
            .eax = sched->current_task->regs.eax,
            .ecx = sched->current_task->regs.ecx,
            .edx = sched->current_task->regs.edx,
            .ebx = sched->current_task->regs.ebx,
            .esp = sched->current_task->regs.esp,
            .ebp = sched->current_task->regs.ebp,
            .esi = sched->current_task->regs.esi,
            .edi = sched->current_task->regs.edi,
            .eflags = sched->current_task->regs.eflags,
            .eip = sched->current_task->regs.eip,
        };
        scheduler_regs_switch(&t);
    }
    else
    {
        regs->eip = (uint32_t)&scheduler_sleep;
    }
}

struct scheduler_task *scheduler_current()
{
    return scheds[cpu_current_core()->core_id].current_task;
}

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
    task->regs.eip = (uint32_t)task->thread->entry;
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
