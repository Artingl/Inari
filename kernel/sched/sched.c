#include <kernel/inari.h>
#include <kernel/sched/sched.h>
#include <kernel/sched/signals.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/interrupts/swi.h>
#include <kernel/interrupts/irq.h>
#include <kernel/errno.h>
#include <kernel/timer.h>
#include <kernel/sync/spinlock.h>

#include <misc/string.h>
#include <misc/list.h>
#include <arch/sys.h>

LIST_HEAD(sched_task_list);

static tid_t sched_last_id = 0xff;
static struct sched_core sched_cores[CONFIG_MAX_CORES];
static struct sched_task *sched_idle_task;
static int sched_initialized = 0;
static spinlock_t sched_lock;

static void __sched_idle()
{
    while (1)
    {
        idle();
    }
}

static struct sched_task *__sched_current_task()
{
    uint32_t core_id = core_id();
    if (core_id > CONFIG_MAX_CORES || !sched_cores[core_id].active || !sched_cores[core_id].task)
        return NULL;
    return sched_cores[core_id].task;
}

void sched_task_preentry()
{
    struct sched_task *task = __sched_current_task();
    if (!task)
        goto end;

    if (task->entrypoint)
        task->entrypoint();
    task->state = SCHED_TASK_DEAD;
end:
    sched_yield();
    idle();
}

static int sched_remove_task(tid_t tid)
{
    struct list_head *pos, *n;
    struct sched_task *entry;

    list_for_each_safe(pos, n, &sched_task_list) {
        entry = list_entry(pos, struct sched_task, list);
        if (!entry) continue;
        if (entry->task_id == tid)
        {
            /* Deallocate the registers and the entry itself */
            if (entry->stack_pointer) kfree(entry->stack_pointer);
            kfree(entry);

            list_del(pos);
            return 0;
        }
    }

    return -1;
}

static struct sched_task *__sched_get_task(tid_t task_id)
{
    struct list_head *pos;
    struct sched_task *entry;

    list_for_each(pos, &sched_task_list) {
        entry = list_entry(pos, struct sched_task, list);
        if (entry && entry->task_id == task_id)
            return entry;
    }

    return NULL;
}

extern void arch_sched_save(struct sched_task *task);
extern void arch_sched_load(struct sched_task *task);

static void sched_save(struct sched_task *task)
{
    uint32_t core_id = core_id();
    if (!task)
        return;
    
    task->reschedules_count++;
    task->cpu_time += timer_get_ticks() - sched_cores[core_id].last_schedule_ticks;

    arch_sched_save(task);
}

static void sched_load(struct sched_task *task)
{
    if (!task)
        return;

    arch_sched_load(task);
}

static void sched_reschedule(struct sched_core *core)
{
    tid_t current_tid = core->task ? core->task->task_id : 0;
    struct list_head *pos, *n;
    struct sched_task *entry, *new_task = (struct sched_task*)NULL;

    /* Update tasks */
    size_t sleep_timeout = (timer_get_ticks() * 1000) / timer_get_resolution() * 1000;
    list_for_each_safe(pos, n, &sched_task_list) {
        entry = list_entry(pos, struct sched_task, list);
        if (!entry) continue;

        if (entry->state == SCHED_TASK_SLEEPING && entry->sleep_timeout <= sleep_timeout)
            entry->state = SCHED_TASK_ACTIVE;
        else if (entry->state == SCHED_TASK_DEAD)
        {
            sched_remove_task(entry->task_id);
            if (core->task == entry)
                core->task = (struct sched_task *)NULL;
        }
    }

    if (core->task)
        sched_save(core->task);

    /* Find next task to schedule */
    list_for_each(pos, &sched_task_list) {
        entry = list_entry(pos, struct sched_task, list);
        if (entry && entry->state == SCHED_TASK_ACTIVE)
        {
            if (entry->task_id > current_tid && entry->task_id != sched_idle_task->task_id)
            {
                core->task = entry;
                return;
            }
        }
    }

    /* Couldn't find suitable task!
     * Try again to find any task to avoid going idle
     */
    list_for_each(pos, &sched_task_list) {
        entry = list_entry(pos, struct sched_task, list);
        if (entry && entry->state == SCHED_TASK_ACTIVE && entry->task_id != sched_idle_task->task_id)
        {
            core->task = entry;
            return;
        }
    }

    core->task = sched_idle_task;
}

static int sched_irq(uint32_t irq, void *dev_id)
{
    sched_call();
    return IRQ_HANDLED;
}

static int sched_swi(uint32_t swi, void *dev_id)
{
    sched_call();
    return SWI_HANDLED;
}

int sched_init()
{
    int ret;
    memset((void*)&sched_cores[0], 0, sizeof(sched_cores));

    sched_idle_task = kmalloc(sizeof(*sched_idle_task));
    if (!sched_idle_task) return -ENOMEM;
    memset((void*)sched_idle_task, 0, sizeof(*sched_idle_task));

    sched_idle_task->task_id = sched_last_id++;
    sched_idle_task->entrypoint = &__sched_idle;
    sched_idle_task->state = SCHED_TASK_ACTIVE;
    list_add_tail(&sched_idle_task->list, &sched_task_list);


    spinlock_init(&sched_lock);
    ret = irq_request(IRQ_TIMER_INTERRUPT, &sched_irq, NULL);
    if (ret != 0) return ret;
    ret = swi_request(SWI_RESCHEDULE, &sched_swi, NULL);
    sched_initialized = 1;
    printk("sched: idle task id: %lu", sched_idle_task->task_id);
    return ret;
}

int sched_is_running()
{
    return sched_initialized;
}

void sched_yield()
{
    interrupts_trigger(SWI_RESCHEDULE);
}

void sched_usleep(tid_t tid, size_t us)
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    struct sched_task *task = __sched_get_task(tid);
    if (!task)
    {
        spin_unlock_irqrestore(&sched_lock, flags);
        return;
    }
    
    task->sleep_timeout = (timer_get_ticks() * 1000) / timer_get_resolution() * 1000 + us;
    task->state = SCHED_TASK_SLEEPING;
    spin_unlock_irqrestore(&sched_lock, flags);
}

void sched_call()
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    uint32_t core_id = core_id();
    if (!sched_cores[core_id].active || !sched_initialized)
    {
        spin_unlock_irqrestore(&sched_lock, flags);
        return;
    }
    
    sched_reschedule(&sched_cores[core_id]);
    if (!sched_cores[core_id].task)
    {
        panic("sched: no tasks to schedule cpu%u", core_id);
    }
    else sched_load(sched_cores[core_id].task);
    sched_cores[core_id].last_schedule_ticks = timer_get_ticks();
    spin_unlock_irqrestore(&sched_lock, flags);
}

int sched_add_task(tid_t *tid, task_entrypoint_t entrypoint)
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    struct sched_task *node = kmalloc(sizeof(*node));
    if (!node) goto err;
    memset((void*)node, 0, sizeof(*node));
    node->task_id = sched_last_id++;
    node->entrypoint = entrypoint;
    node->state = SCHED_TASK_ACTIVE;
    list_add_tail(&node->list, &sched_task_list);
    if (tid)
        *tid = node->task_id;
    spin_unlock_irqrestore(&sched_lock, flags);
    return 0;
err:
    spin_unlock_irqrestore(&sched_lock, flags);
    return -ENOMEM;
}

int sched_get_task(tid_t task_id, struct sched_task **task)
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    if (!task) goto err;
    struct sched_task *_task = __sched_get_task(task_id);
    if (!_task) goto err;
    *task = _task;
    spin_unlock_irqrestore(&sched_lock, flags);
    return 0;
err:
    spin_unlock_irqrestore(&sched_lock, flags);
    return -1;
}

int sched_signal_task(tid_t tid, uint32_t signo)
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    struct sched_task *task = __sched_get_task(tid);
    if (!task) goto err;
    
    switch (signo)
    {
        case SIGKILL:
        case SIGSTOP:
            printk("sched%u: signal %u", task->task_id, signo);
            task->state = SCHED_TASK_DEAD;
            break;

        default:
            if (!task->signal_handler || task->inside_signal)
            {
                printk("sched%u: signal %u", task->task_id, signo);
                task->state = SCHED_TASK_DEAD;
                break;
            }
            /* TODO: implement other signals */
    }

    spin_unlock_irqrestore(&sched_lock, flags);
    return 0;
err:
    spin_unlock_irqrestore(&sched_lock, flags);
    return -1;
}

int sched_current_task(tid_t *tid)
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    struct sched_task *task = __sched_current_task();
    if (!task || !tid) goto err;
    *tid = task->task_id;
    spin_unlock_irqrestore(&sched_lock, flags);
    return 0;
err:
    spin_unlock_irqrestore(&sched_lock, flags);
    return -1;
}

void sched_stop()
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    size_t i;
    for (i = 0; i < CONFIG_MAX_CORES; i++)
        sched_cores[i].active = 0;
    spin_unlock_irqrestore(&sched_lock, flags);
}

void sched_enter_core()
{
    uint32_t core_id = core_id();

    if (core_id >= CONFIG_MAX_CORES)
    {
        printk("sched: ignoring out-of-bounds core id %u", core_id);
        /* Just halt it for now */
        halt();
    }

    sched_cores[core_id].core_id = core_id;
    sched_cores[core_id].active = 1;
    sched_cores[core_id].task = (struct sched_task*)NULL;
    
    printk("sched: running on cpu%u", core_id);
    idle();
}