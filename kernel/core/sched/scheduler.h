#pragma once

#include <kernel/libc/typedefs.h>
#include <kernel/core/lock/spinlock.h>

// #define SCHEDULER_DEBUG

#define TASK_STATE_ACTIVE   0x00
#define TASK_STATE_SLEEPING 0x01
#define TASK_STATE_DEAD     0x02

// Defines how many timer IRQs must happen before we forcefully switch the task
#define SCHEDULER_TASK_TIMEOUT 1

typedef size_t tid_t; // task id

struct sched_task
{
    void *entrypoint;
    void *stack_base;
    struct sched_core *attached_core;

    uint8_t state;
    tid_t tid;

    // buffer for any arch to store its regs in
    uint8_t regs[0xff];
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
