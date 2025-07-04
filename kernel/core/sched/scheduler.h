#pragma once

#include <kernel/libc/typedefs.h>
#include <kernel/core/lock/spinlock.h>

#ifdef CONFIG_ARCH_I686
    #include <kernel/arch/i686/impl.h>
#endif

#define TASK_STATE_ACTIVE   0x00
#define TASK_STATE_SLEEPING 0x01
#define TASK_STATE_DEAD     0x02

// Defines how many timer IRQs must happen before we forcefully switch the task
#define SCHEDULER_TASK_TIMEOUT 1//10

typedef size_t tid_t; // task id

struct sched_task
{
    void *entrypoint;
    void *stack_base;
    struct sched_core *attached_core;

    uint8_t state;
    tid_t tid;

#ifdef CONFIG_ARCH_I686
    struct {
        uint32_t esp;
        uint32_t ss;
    } regs;
#endif
};

struct sched_core
{
    uint8_t is_running;
    int16_t task_timeout_counter;
    struct sched_task *current_task;
};

struct sched_task *scheduler_find_task(tid_t tid);
tid_t scheduler_create_task(void *entrypoint);
void scheduler_kill_task(tid_t tid);
int scheduler_init();
int scheduler_enter();
void scheduler_shutdown();
