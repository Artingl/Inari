#pragma once

#include <kernel/libc/typedefs.h>
#include <kernel/core/lock/spinlock.h>

// #define SCHEDULER_DEBUG

#define TASK_STATE_ACTIVE   0x00
#define TASK_STATE_SLEEPING 0x01
#define TASK_STATE_DEAD     0x02

// Defines how many timer IRQs must happen before we forcefully switch the task
#define SCHEDULER_TASK_TIMEOUT 5

typedef size_t tid_t; // task id

struct sched_task
{
    void *entrypoint;
    void *stack_base;
    void *argument;
    struct sched_core *attached_core;

    uint8_t state;
    uint32_t sleeping_timeout;
    tid_t tid;

    // buffer for any arch to store its regs in
    uint8_t regs[0xff];
};

struct sched_core
{
    uint8_t is_running;
    int16_t task_timeout_counter;
    size_t last_task_index;
    struct sched_task *current_task;
};

struct sched_task *scheduler_find_task(tid_t tid);
struct sched_task *scheduler_get_current_task();
tid_t scheduler_create_task(void *entrypoint, void *argument);
void scheduler_kill_task(tid_t tid);
void scheduler_yield();
int scheduler_init();
int scheduler_enter();
void scheduler_shutdown();