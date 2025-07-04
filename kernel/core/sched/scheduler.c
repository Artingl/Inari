#include <kernel/kernel.h>
#include <kernel/core/sched/scheduler.h>
#include <kernel/libc/string.h>
#include <kernel/list/dynlist.h>
#include <kernel/driver/memory/memory.h>
#include <kernel/driver/interrupt/interrupt.h>

// TODO: move all the x86 dependent code to its arch folder
#include <kernel/arch/i686/impl.h>

// #define SCHEDULER_DEBUG

int __scheduler_enabled = 0;
size_t __scheduler_last_tid = 0;

struct sched_core sched_cores[KERN_MAX_CORES];
dynlist_t sched_tasks = {0};
spinlock_t sched_tasks_lock = {0};

static void scheduler_core_idle()
{
    __halt();
}

static struct sched_task *scheduler_get_current_task()
{
    if (!sched_cores[0].is_running) return NULL;
    return sched_cores[0].current_task;
}

static void scheduler_task_entrypoint()
{
    // struct sched_task *task;
    // __asm__ volatile("movl %%eax,%0" : "=r"(task));

    struct sched_task *task = scheduler_get_current_task();
    if (task == NULL)
    {
        panic("sched: task entrypoint, got NULL task");
        __disable_int();
        __halt();
        return;
    }

    // Call the task's entrypoint
    ((void(*)(void))task->entrypoint)();

    // Kill the task and idle the core if we reached the end of the entrypoint
    scheduler_kill_task(task->tid);
    scheduler_core_idle();
}

static void scheduler_attach_task(struct sched_core *core)
{
    size_t i;
    struct sched_task *task, *last_task;

    spinlock_acquire(&sched_tasks_lock);

    if (dynlist_size(sched_tasks) == 0)
    {
        panic("sched: no tasks to schedule!");
        __disable_int();
        __halt();
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
    spinlock_release(&sched_tasks_lock);
}

void scheduler_reschedule(struct regs32 *regs)
{
    size_t i;
    struct sched_core *core;

    // TODO: SMP implementation
#if 0
    for (i = 0; i < KERN_MAX_CORES; i++) {
        core = &sched_cores[i];
        scheduler_attach_task(core);
        if (!core->is_running) continue;

        // Attach task to core if it's idling or the timeout expires
        if (core->current_task == NULL || core->task_timeout_counter <= 0)
        {
            scheduler_attach_task(core);
        }

        core->task_timeout_counter--;
        spinlock_release(&core->lock);
    }
#else

    core = &sched_cores[0];
    if (!core->is_running) return;
    core->task_timeout_counter--;

    // Save state of current task if we have any
    if (core->current_task != NULL)
    {
        #ifdef SCHEDULER_DEBUG
            printk("sched: saving current task tid=%d esp=%p -> %p",
                core->current_task->tid,
                core->current_task->regs.esp, regs->esp);
        #endif

        core->current_task->regs.ss = 0x10; // regs->ss;
        core->current_task->regs.esp = regs->esp_pushad; // In idt.asm we push 2 dwords, so we need to add 8
    }

    // Attach task to core if it's idling or the timeout expires
    if (core->current_task == NULL
        || core->current_task->state != TASK_STATE_ACTIVE
        || core->task_timeout_counter <= 0)
    {

        scheduler_attach_task(core);

        if (core->current_task != NULL) {
            #ifdef SCHEDULER_DEBUG
                printk("sched: switching to task tid=%d esp=%p -> %p",
                    core->current_task->tid,
                    core->current_task->regs.esp, regs->esp);
            #endif

            // Perform complete context switch to the task
            __asm__ volatile(
                // Load pointer to task registers struct into EAX
                "mov %0, %%esp\n\t"
                "popa\n\t"
                "add $8, %%esp\n\t"
                "iret\n\t"
                :
                : "m"(core->current_task->regs.esp)
                : "memory"
            );
        }
    }
#endif
}

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
    
    // Initialize all registers
    task->regs.esp = (unsigned int)(task->stack_base + KERN_STACK_SIZE);
    task->regs.esp &= ~0xF;
    task->regs.ss = 0x10;    // Kernel data segment

    // Fill the stack with registers that will be later used
    uint32_t *stack_array = (uint32_t *)task->regs.esp;
    memset((uint8_t*)task->regs.esp, 0, KERN_STACK_SIZE);
    uint32_t offset = 1;

    *(stack_array - (offset++)) = 0x202;            // set eflags value
    *(stack_array - (offset++)) = 0x08;             // set CS value
    *(stack_array - (offset++)) = (unsigned int)&scheduler_task_entrypoint; // set EIP value

    *(stack_array - (offset++)) = 0;
    *(stack_array - (offset++)) = 0;

    *(stack_array - (offset++)) = 0;
    *(stack_array - (offset++)) = 0;
    *(stack_array - (offset++)) = 0;
    *(stack_array - (offset++)) = 0;
    *(stack_array - (offset++)) = task->regs.esp;
    *(stack_array - (offset++)) = task->regs.esp;
    *(stack_array - (offset++)) = 0;
    *(stack_array - (offset++)) = 0;

    task->regs.esp -= (offset - 1) * 4;
end:
    dynlist_append(sched_tasks, task);
    spinlock_release(&sched_tasks_lock);

    return tid;
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

    __scheduler_enabled = 1;
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
    scheduler_core_idle();

    return 0;
}

void scheduler_shutdown()
{
    size_t i;
    __scheduler_enabled = 0;

    // Shutdown all cores
    for (i = 0; i < KERN_MAX_CORES; i++)
    {
        sched_cores[i].is_running = 0;
    }
}

