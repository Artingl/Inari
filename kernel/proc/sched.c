#include <kernel/inari.h>
#include <kernel/proc/sched.h>
#include <kernel/proc/signals.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/interrupts/swi.h>
#include <kernel/interrupts/irq.h>
#include <kernel/errno.h>
#include <kernel/timer.h>
#include <kernel/sync/spinlock.h>

#include <arch/paging.h>
#include <misc/string.h>
#include <misc/list.h>
#include <arch/sys.h>

static LIST_HEAD(sched_task_list);

static tid_t sched_last_id = 0x01;
static struct sched_core sched_cores[CONFIG_MAX_CORES];
static struct thread *sched_idle_task;
static int sched_initialized = 0;
static spinlock_t sched_lock = {0};

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
int arch_sched_load(struct thread *task);

static void sched_save(struct thread *task)
{
    uint32_t core_id = core_id();
    if (!task || task->flags & SCHED_FLAG_SIGRETURN)
    {
        task->flags &= ~SCHED_FLAG_SIGRETURN;
        return;
    }
    
    task->reschedules_count++;
    task->cpu_time += timer_get_ticks() - sched_cores[core_id].last_schedule_ticks;

    arch_sched_save(task);
}

static int sched_load(struct thread *task)
{
    if (!task)
        return -1;
    return arch_sched_load(task);
}

static void sched_update_tasks(struct sched_core *core)
{
    tid_t current_tid = core->task ? core->task->tid : 0;
    struct list_head *pos, *n;
    struct thread *entry;

    /* Update tasks */
    size_t sleep_timeout = (timer_get_ticks() * 1000) / timer_get_resolution() * 1000;
    list_for_each_safe(pos, n, &sched_task_list) {
        entry = list_entry(pos, struct thread, list);
        if (!entry) continue;

        if (entry->state == SCHED_TASK_SLEEPING && entry->sleep_timeout <= sleep_timeout)
            entry->state = SCHED_TASK_ACTIVE;
        /* Note: When we'll have SMP, the deallocation below will break.
                    Such thing can happen: core0 uses kern stack of below process,
                    unlocks lock, core1 immediately gains lock and deallocates the stack
                    while it is still in use */
        else if (entry->state == SCHED_TASK_DEAD && core->task != entry)
        {
            list_del(pos);
            if (entry->kernel_stack_pointer && entry->vmem) vmm_free_pages(arch_get_kernel_pagedir(), entry->kernel_stack_pointer, (CONFIG_STACK_SIZE >> 12) + 1);
            if (entry->thread_stack_pointer && entry->vmem) vmm_free_pages(entry->vmem, entry->thread_stack_pointer, (CONFIG_STACK_SIZE >> 12) + 1);
            sched_handle_death(entry);
            kfree((void*)entry);
        }
    }
}

static void sched_reschedule(struct sched_core *core)
{
    tid_t current_tid = core->task ? core->task->tid : 0;
    struct list_head *pos;
    struct thread *entry, *new_task = (struct thread*)NULL;

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

    /* TODO: if ever SMP will be implemented, each core must have its own idle task */
    sched_idle_task->vmem = arch_get_kernel_pagedir();
    sched_idle_task->tid = sched_last_id++;
    sched_idle_task->entrypoint = &__sched_idle;
    sched_idle_task->state = SCHED_TASK_ACTIVE;
    sched_idle_task->sig_saved_stack = NULL;
    sched_idle_task->flags = 0;
    sched_idle_task->reschedules_count = 1;
    list_add_tail(&sched_idle_task->list, &sched_task_list);

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
    uint32_t core_id = core_id();
    if (!sched_cores[core_id].active || !sched_initialized)
        return;

    spin_lock_irqsave(&sched_lock, flags);
    int load_result = -1, tries = 0;
    sched_update_tasks(&sched_cores[core_id]);
    do {
        sched_reschedule(&sched_cores[core_id]);
        if (sched_cores[core_id].task)
            load_result = sched_load(sched_cores[core_id].task);
    } while (load_result != 0 && tries++ < 5);
    if (!sched_cores[core_id].task)
        panic("sched: no tasks to schedule cpu%u", core_id);
    sched_cores[core_id].last_schedule_ticks = timer_get_ticks();
    spin_unlock_irqrestore(&sched_lock, flags);
}

int sched_create_thread(tid_t *tid, thread_entrypoint_t entrypoint, pagedir_t *vmem, thread_cleanup_t cleanup_handler, struct process *proc_data)
{
    uint32_t flags;
    struct thread *node = kmalloc(sizeof(*node));
    if (!node) return -ENOMEM;
    if (!vmem) vmem = arch_get_kernel_pagedir();
    node->vmem = vmem;
    node->tid = sched_last_id++;
    node->entrypoint = entrypoint;
    node->cleanup_handler = cleanup_handler;
    node->proc_data = proc_data;
    node->sig_saved_stack = NULL;
    node->reschedules_count = 0xff;    // start with larger value for correct cpu usage calculation for short-lived tasks
    node->flags = 0;
    node->sleep_timeout = (timer_get_ticks() * 1000) / timer_get_resolution() * 1000 + 0x1000;
    node->state = SCHED_TASK_SLEEPING;
    spin_lock_irqsave(&sched_lock, flags);
    list_add_tail(&node->list, &sched_task_list);
    spin_unlock_irqrestore(&sched_lock, flags);
    if (tid)
        *tid = node->tid;
    return 0;
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

int sched_kill_thread(tid_t tid)
{
    uint32_t flags, res = 0;
    spin_lock_irqsave(&sched_lock, flags);
    struct thread *task = __sched_get_thread(tid);
    if (!task)
    {
        res = -EINVAL;
        goto end;
    }
    task->state = SCHED_TASK_DEAD;
end:
    spin_unlock_irqrestore(&sched_lock, flags);
    return res;
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
    size_t i;
    for (i = 0; i < CONFIG_MAX_CORES; i++)
        sched_cores[i].active = 0;
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
    enable_int();
    idle();
}