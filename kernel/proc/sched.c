#include <kernel/inari.h>
#include <kernel/proc/sched.h>
#include <kernel/proc/signals.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/page.h>
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
static struct thread *sched_idle_task;
static int sched_initialized = 0;
static spinlock_t sched_lock;

static void __sched_idle()
{
    while (1)
    {
        idle();
    }
}

struct thread *__sched_current_thread()
{
    uint32_t core_id = core_id();
    if (core_id >= CONFIG_MAX_CORES || !sched_cores[core_id].active || !sched_cores[core_id].task)
        return NULL;
    return sched_cores[core_id].task;
}

static void sched_handle_death(struct thread *th)
{
    if (!th) return;

    if (th->cleanup_handler)
        th->cleanup_handler(th, th->proc_data);

    /* Panic if critical process died */
    if (th->flags & SCHED_FLAG_SYSTEM)
    {
        spin_unlock(&sched_lock); // don't forget to unlock!
        panic("sched: critical process died");
    }
}

void sched_thread_preentry()
{
    struct thread *task = __sched_current_thread();
    if (!task)
        goto end;

    if (task->entrypoint)
        task->entrypoint(task->proc_data);
    task->state = SCHED_TASK_DEAD;
end:
    sched_yield();
    idle();
}

static int sched_remove_thread(tid_t tid)
{
    struct list_head *pos, *n;
    struct thread *entry;
    pagedir_t prev_dir = NULL;

    list_for_each_safe(pos, n, &sched_task_list) {
        entry = list_entry(pos, struct thread, list);
        if (!entry) continue;
        if (entry->tid == tid)
        {
            /* Deallocate the stack and the entry itself */
            if (entry->stack_pointer)
            {
                prev_dir = page_get_dir();
                page_switch_dir(entry->vmem);
                kfree(entry->stack_pointer);
                page_switch_dir(prev_dir);
            }
            
            kfree(entry);

            list_del(pos);
            return 0;
        }
    }

    return -1;
}

struct thread *__sched_get_thread(tid_t tid)
{
    struct list_head *pos;
    struct thread *entry;

    list_for_each(pos, &sched_task_list) {
        entry = list_entry(pos, struct thread, list);
        if (entry && entry->tid == tid)
            return entry;
    }

    return NULL;
}

void arch_sched_save(struct thread *task);
void arch_sched_load(struct thread *task);

static void sched_save(struct thread *task)
{
    uint32_t core_id = core_id();
    if (!task)
        return;
    
    task->reschedules_count++;
    task->cpu_time += timer_get_ticks() - sched_cores[core_id].last_schedule_ticks;

    arch_sched_save(task);
}

static void sched_load(struct thread *task)
{
    if (!task)
        return;
    arch_sched_load(task);
}

static void sched_reschedule(struct sched_core *core)
{
    tid_t current_tid = core->task ? core->task->tid : 0;
    struct list_head *pos, *n;
    struct thread *entry, *new_task = (struct thread*)NULL;

    /* Update tasks */
    size_t sleep_timeout = (timer_get_ticks() * 1000) / timer_get_resolution() * 1000;
    list_for_each_safe(pos, n, &sched_task_list) {
        entry = list_entry(pos, struct thread, list);
        if (!entry) continue;

        if (entry->state == SCHED_TASK_SLEEPING && entry->sleep_timeout <= sleep_timeout)
            entry->state = SCHED_TASK_ACTIVE;
        else if (entry->state == SCHED_TASK_DEAD)
        {
            sched_handle_death(entry);
            sched_remove_thread(entry->tid);
            if (core->task == entry)
                core->task = (struct thread *)NULL;
        }
    }

    if (core->task)
        sched_save(core->task);

    /* Find next task to schedule */
    list_for_each(pos, &sched_task_list) {
        entry = list_entry(pos, struct thread, list);
        if (entry && entry->state == SCHED_TASK_ACTIVE)
        {
            if (entry->tid > current_tid && entry->tid != sched_idle_task->tid)
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
        entry = list_entry(pos, struct thread, list);
        if (entry && entry->state == SCHED_TASK_ACTIVE && entry->tid != sched_idle_task->tid)
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

    sched_idle_task->vmem = get_kernel_pagedir();
    sched_idle_task->tid = sched_last_id++;
    sched_idle_task->entrypoint = &__sched_idle;
    sched_idle_task->state = SCHED_TASK_ACTIVE;
    sched_idle_task->flags = 0;
    list_add_tail(&sched_idle_task->list, &sched_task_list);

    spinlock_init(&sched_lock);
    ret = irq_request(IRQ_TIMER_INTERRUPT, &sched_irq, NULL);
    if (ret != 0) return ret;
    ret = swi_request(SWI_RESCHEDULE, &sched_swi, NULL);
    if (ret != 0) return ret;
    ret = swi_request(SWI_SYSCALL, &sched_swi, NULL);
    if (ret == 0)
    {
        sched_initialized = 1;
        printk("sched: idle task id: %lu", sched_idle_task->tid);
    }
    
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

int sched_usleep(tid_t tid, size_t us)
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    struct thread *task = __sched_get_thread(tid);
    if (!task)
    {
        spin_unlock_irqrestore(&sched_lock, flags);
        return -EINVAL;
    }
    
    task->sleep_timeout = (timer_get_ticks() * 1000) / timer_get_resolution() * 1000 + us;
    task->state = SCHED_TASK_SLEEPING;
    spin_unlock_irqrestore(&sched_lock, flags);
    return 0;
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

int sched_create_thread(tid_t *tid, thread_entrypoint_t entrypoint, pagedir_t vmem, thread_cleanup_t cleanup_handler, void *proc_data)
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    struct thread *node = kmalloc(sizeof(*node));
    if (!node) goto err;
    if (!vmem) vmem = get_kernel_pagedir();
    memset((void*)node, 0, sizeof(*node));
    node->vmem = vmem;
    node->tid = sched_last_id++;
    node->entrypoint = entrypoint;
    node->cleanup_handler = cleanup_handler;
    node->proc_data = proc_data;
    node->flags = 0;
    node->sleep_timeout = (timer_get_ticks() * 1000) / timer_get_resolution() * 1000 + 0x1000;
    node->state = SCHED_TASK_SLEEPING;
    list_add_tail(&node->list, &sched_task_list);
    if (tid)
        *tid = node->tid;
    spin_unlock_irqrestore(&sched_lock, flags);
    return 0;
err:
    spin_unlock_irqrestore(&sched_lock, flags);
    return -ENOMEM;
}

int sched_get_thread(tid_t tid, struct thread **task)
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    if (!task) goto err;
    struct thread *_task = __sched_get_thread(tid);
    if (!_task) goto err;
    *task = _task;
    spin_unlock_irqrestore(&sched_lock, flags);
    return 0;
err:
    spin_unlock_irqrestore(&sched_lock, flags);
    return -1;
}

int sched_thread_set_state(tid_t tid, int state)
{
    uint32_t irq_flags;
    spin_lock_irqsave(&sched_lock, irq_flags);
    struct thread *task = __sched_get_thread(tid);
    if (!task) goto err;
    task->state = state;
    spin_unlock_irqrestore(&sched_lock, irq_flags);
    return 0;
err:
    spin_unlock_irqrestore(&sched_lock, irq_flags);
    return -1;
}


int sched_thread_set_flags(tid_t tid, uint32_t flags)
{
    uint32_t irq_flags;
    spin_lock_irqsave(&sched_lock, irq_flags);
    struct thread *task = __sched_get_thread(tid);
    if (!task) goto err;
    task->flags = flags;
    spin_unlock_irqrestore(&sched_lock, irq_flags);
    return 0;
err:
    spin_unlock_irqrestore(&sched_lock, irq_flags);
    return -1;
}

int sched_signal_thread(tid_t tid, uint32_t signo)
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    struct thread *task = __sched_get_thread(tid);
    if (!task) goto err;
    
    switch (signo)
    {
        case SIGKILL:
        case SIGSTOP:
            // printk("sched%llu: signal %u", task->tid, signo);
            task->state = SCHED_TASK_DEAD;
            break;

        default:
            if (!task->signal_handler || task->flags & SCHED_FLAG_IN_SIGNAL)
            {
                // printk("sched%llu: signal %u", task->tid, signo);
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

int sched_current_thread(tid_t *tid)
{
    uint32_t flags;
    spin_lock_irqsave(&sched_lock, flags);
    struct thread *task = __sched_current_thread();
    if (!task || !tid) goto err;
    *tid = task->tid;
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

    printk("sched: running on cpu%u", core_id);

    sched_cores[core_id].core_id = core_id;
    sched_cores[core_id].active = 1;
    sched_cores[core_id].task = (struct thread*)NULL;
    idle();
}