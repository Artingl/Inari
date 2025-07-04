#include <kernel/kernel.h>
#include <kernel/core/sched/scheduler.h>
#include <kernel/libc/string.h>
#include <kernel/list/dynlist.h>
#include <kernel/driver/memory/memory.h>
#include <kernel/driver/interrupt/interrupt.h>

size_t __scheduler_last_tid = 0;

struct sched_core sched_cores[KERN_MAX_CORES];
int scheduler_enabled_flag = 0;

dynlist_t sched_tasks = {0};
spinlock_t sched_tasks_lock = {0};

extern void arch_scheduler_core_idle();
extern void arch_scheduler_load_current_task(struct sched_core *core, void *regs_ptr);
extern void arch_scheduler_save_current_task(struct sched_core *core, void *regs_ptr);
extern void arch_scheduler_init_task(struct sched_task *task);

tid_t scheduler_create_task(void *entrypoint)
{
    spinlock_acquire(&sched_tasks_lock);

    tid_t tid = __scheduler_last_tid++;
    struct sched_task *task = kmalloc(sizeof(struct sched_task));
    memset(task, 0, sizeof(struct sched_task));

    task->tid = tid;
    task->state = TASK_STATE_ACTIVE;
    task->entrypoint = (void*)entrypoint;
    task->stack_base = kmalloc(KERN_STACK_SIZE);
    
    arch_scheduler_init_task(task);
end:
    dynlist_append(sched_tasks, task);
    spinlock_release(&sched_tasks_lock);

    return tid;
}

static struct sched_task *scheduler_get_current_task()
{
    if (!sched_cores[0].is_running) return NULL;
    return sched_cores[0].current_task;
}

void scheduler_task_entrypoint()
{
    struct sched_task *task = scheduler_get_current_task();
    if (task == NULL)
    {
        panic("sched: task entrypoint, got NULL task");
        return;
    }

    // Call the task's entrypoint
    ((void(*)(void))task->entrypoint)();

    // Kill the task and idle the core if we reached the end of the entrypoint
    scheduler_kill_task(task->tid);
    arch_scheduler_core_idle();
}

static void scheduler_attach_task(struct sched_core *core)
{
    size_t i;
    struct sched_task *task, *last_task;

    if (dynlist_size(sched_tasks) == 0)
    {
        panic("sched: no tasks to schedule!");
        return;
    }

    last_task = core->current_task;
    if (last_task != NULL) {
        last_task->attached_core = NULL;
        core->current_task = NULL;
    }

    // Get next task for the core to execute
    for (i = 0; i < dynlist_size(sched_tasks); i++)
    {
        task = dynlist_get(sched_tasks, i, struct sched_task*);

        // Check if the task is available
        if (task->attached_core == NULL && task->state == TASK_STATE_ACTIVE)
        {
            if (last_task == NULL || dynlist_size(sched_tasks) <= 1)
            {
                task->attached_core = core;
                core->current_task = task;
                break;
            }
            else if (last_task->tid < task->tid || last_task->tid >= dynlist_size(sched_tasks) - 1)
            {
                task->attached_core = core;
                core->current_task = task;
                break;
            }
        }
        else if (task->state == TASK_STATE_DEAD && task->attached_core == NULL) {
            // Cleanup the dead task
            #ifdef SCHEDULER_DEBUG
                printk("sched: removing dead task: tid=%d", task->tid);
            #endif
            dynlist_remove(sched_tasks, i);
            kfree(task->stack_base);
            kfree(task);
            break;
        }
    }

    core->task_timeout_counter = SCHEDULER_TASK_TIMEOUT;
}

void scheduler_reschedule(void *regs_ptr)
{
    spinlock_acquire(&sched_tasks_lock);
    size_t i;
    struct sched_core *core;

    core = &sched_cores[0];
    if (!core->is_running) goto end;
    core->task_timeout_counter--;

    // Save state of current task if we have any
    if (core->current_task != NULL)
    {
        arch_scheduler_save_current_task(core, regs_ptr);
    }

    // Attach task to core if it's idling or the timeout expires
    if (core->current_task == NULL
        || core->current_task->state != TASK_STATE_ACTIVE
        || core->task_timeout_counter <= 0)
    {

        scheduler_attach_task(core);

        if (core->current_task != NULL) {
            // Do not forget to release the lock!
            // The load_current_task function is noreturn
            spinlock_release(&sched_tasks_lock);
            arch_scheduler_load_current_task(core, regs_ptr);
        }
    }

end:
    spinlock_release(&sched_tasks_lock);
}

struct sched_task *scheduler_find_task(tid_t tid)
{
    size_t i;
    struct sched_task *task, *found_task = NULL;
    spinlock_acquire(&sched_tasks_lock);

    for (i = 0; i < dynlist_size(sched_tasks); i++)
    {
        task = dynlist_get(sched_tasks, i, struct sched_task*);
        if (task != NULL && task->tid == tid)
        {
            found_task = task;
            goto end;
        }
    }

end:
    spinlock_release(&sched_tasks_lock);
    return found_task;
}

void scheduler_kill_task(tid_t tid)
{
    size_t i;
    struct sched_task *task;
    spinlock_acquire(&sched_tasks_lock);
    
    // Find the task manually instead of calling scheduler_find_task to avoid deadlock
    for (i = 0; i < dynlist_size(sched_tasks); i++)
    {
        task = dynlist_get(sched_tasks, i, struct sched_task*);
        if (task != NULL && task->tid == tid)
        {
            task->state = TASK_STATE_DEAD;
            break;
        }
    }
    
    spinlock_release(&sched_tasks_lock);
}

int scheduler_init()
{
    // Fill the cores array with zeros
    memset((void*)&sched_cores, 0, sizeof(sched_cores));
    scheduler_enabled_flag = 1;
    return 0;
}

int scheduler_enter()
{
    uint16_t core_id = 0;

    if (core_id >= KERN_MAX_CORES)
    {
        return -1;
    }

    // Idle the core while waiting for the scheduling
    printk("sched: running on core %u", core_id);
    sched_cores[core_id].is_running = 1;
    arch_scheduler_core_idle();

    return 0;
}

void scheduler_shutdown()
{
    size_t i;
    scheduler_enabled_flag = 0;

    // Shutdown all cores
    for (i = 0; i < KERN_MAX_CORES; i++)
    {
        sched_cores[i].is_running = 0;
    }
}

