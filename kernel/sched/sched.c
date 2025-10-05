#include <kernel/inari.h>
#include <kernel/sched/sched.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/interrupts/swi.h>
#include <kernel/interrupts/irq.h>
#include <kernel/errno.h>

#include <misc/string.h>
#include <misc/list.h>
#include <arch/sys.h>

LIST_HEAD(sched_task_list);

struct sched_task
{
    tid_t task_id;
    task_entrypoint_t entrypoint;
    uint8_t state;
    void *stack_pointer;
    void *registers;

    struct list_head list;
};

struct sched_core
{
    uint8_t active;
    uint32_t core_id;
    struct sched_task *task;
    struct sched_task *prev_task;
};

static tid_t sched_last_id = 0xff;
static struct sched_core sched_cores[CONFIG_MAX_CORES];

static struct sched_task sched_idle_task;

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

static void sched_task_preentry()
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

static struct sched_task *sched_get_task(tid_t task_id)
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

static void sched_save(struct sched_task *task)
{
    /* TODO: move this code to the architecture-specific code */
    struct interrupt_frame *frame = interrupt_frame();
    if (!frame || !task || !task->registers)
        return;
    
    /* Do not save idle task */
    if (task->task_id == SCHED_IDLE_TASK_ID)
        return;

    memcpy(task->registers, frame->registers.base, frame->registers.size);
}

static void sched_enter(struct sched_task *task)
{
    /* TODO: move this code to the architecture-specific code */
    struct interrupt_frame *frame = interrupt_frame();
    if (!frame || !task)
        return;
    
    if (!task->registers)
    {
        if (task->stack_pointer == NULL)
            task->stack_pointer = kmalloc(CONFIG_STACK_SIZE);

        /* If the registers array is not initialized, that's the first time this task is scheduled */
        task->registers = kmalloc(frame->registers.size);
        memcpy(task->registers, frame->registers.base, frame->registers.size);
        modify_register_array(task->registers, REGISTER_IP, &sched_task_preentry);
        modify_register_array(task->registers, REGISTER_SP, (void*)((uintptr_t)task->stack_pointer + (uintptr_t)CONFIG_STACK_SIZE));
    }

    memcpy(frame->registers.base, task->registers, frame->registers.size);
}

static void sched_reschedule(struct sched_core *core)
{
    tid_t current_tid = core->task ? core->task->task_id : 0;
    struct list_head *pos, *n;
    struct sched_task *entry,
        *first_task = (struct sched_task*)NULL,
        *new_task = (struct sched_task*)NULL;

    /* Cleanup dead tasks */
    list_for_each_safe(pos, n, &sched_task_list) {
        entry = list_entry(pos, struct sched_task, list);
        if (entry && entry->state == SCHED_TASK_DEAD)
        {
            sched_remove_task(entry->task_id);
            if (core->task == entry)
                core->task = (struct sched_task *)NULL;
        }
    }

    core->prev_task = core->task;
    if (core->task)
        sched_save(core->task);

    list_for_each(pos, &sched_task_list) {
        entry = list_entry(pos, struct sched_task, list);
        if (entry && entry->state == SCHED_TASK_ACTIVE)
        {
            if (!first_task) first_task = entry;
            if (entry->task_id > current_tid)
            {
                new_task = entry;
                break;
            }
        }
    }

    if (first_task)
    {
        if (!new_task)
            new_task = first_task;

        core->task = new_task;
    }
    else core->task = (struct sched_task*)NULL;
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

    sched_idle_task.task_id = SCHED_IDLE_TASK_ID;
    sched_idle_task.entrypoint = &__sched_idle;
    sched_idle_task.registers = NULL;
    sched_idle_task.stack_pointer = NULL;
    sched_idle_task.state = SCHED_TASK_ACTIVE;

    ret = irq_request(IRQ_TIMER_INTERRUPT, &sched_irq, NULL);
    if (!ret) return ret;
    ret = swi_request(SWI_RESCHEDULE, &sched_swi, NULL);
    return ret;
}

void sched_yield()
{
    interrupts_trigger(SWI_RESCHEDULE);
}

void sched_call()
{
    uint32_t core_id = core_id();
    if (!sched_cores[core_id].active)
        return;
    
    sched_reschedule(&sched_cores[core_id]);
    if (!sched_cores[core_id].task)
    {
        if (sched_cores[core_id].prev_task != &sched_idle_task)
            printk("sched: no tasks to schedule cpu%u", core_id);
        sched_cores[core_id].task = &sched_idle_task;
        sched_enter(&sched_idle_task);
    }
    else sched_enter(sched_cores[core_id].task);
}

int sched_add_task(tid_t *tid, task_entrypoint_t entrypoint)
{
    struct sched_task *node = kmalloc(sizeof(*node));
    if (!node) return -ENOMEM;
    node->task_id = sched_last_id++;
    node->entrypoint = entrypoint;
    node->registers = NULL;
    node->stack_pointer = NULL;
    node->state = SCHED_TASK_ACTIVE;
    list_add_tail(&node->list, &sched_task_list);
    if (tid)
        *tid = node->task_id;
    return 0;
}

int sched_remove_task(tid_t tid)
{
    struct list_head *pos, *n;
    struct sched_task *entry;

    list_for_each_safe(pos, n, &sched_task_list) {
        entry = list_entry(pos, struct sched_task, list);
        if (entry->task_id == tid)
        {
            list_del(pos);     // unlink

            /* Deallocate the registers and the entry itself */
            kfree(entry->registers);
            kfree(entry->stack_pointer);
            kfree(entry);
            return 0;
        }
    }

    return -1;
}

int sched_signal_task(tid_t tid, uint32_t signal)
{
    struct sched_task *task = sched_get_task(tid);
    if (!task) return -1;
    
    switch (signal)
    {
        case SCHED_SIGNAL_TERM:
            printk("sched: task %u terminated", task->task_id);
            task->state = SCHED_TASK_DEAD;
            break;

        default:
            /* TODO: implement other signals */
    }

    return 0;
}

tid_t sched_current_task()
{
    struct sched_task *task = __sched_current_task();
    if (!task) return 0;
    return task->task_id;
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

    sched_cores[core_id].core_id = core_id;
    sched_cores[core_id].active = 1;
    sched_cores[core_id].task = (struct sched_task*)NULL;
    
    printk("sched: running on cpu%u", core_id);
    idle();
}