#pragma once

#include <drivers/cpu/cpu.h>
#include <drivers/impl.h>

#include <kernel/lock/spinlock.h>
#include <kernel/scheduler/thread.h>

struct scheduler_task;

struct scheduler {
    volatile bool alive;
    struct scheduler_task *current_task;
    size_t counter;
    size_t last_task_idx;
};

struct scheduler_task
{
    struct scheduler *owner;

    int32_t task_id;
    regs32_t regs;

    spinlock_t lock;
    thread_t *thread;
};

void scheduler_init();
void scheduler_shutdown();
bool scheduler_alive();

struct scheduler_task *scheduler_current();

void scheduler_task_cleanup(struct scheduler_task *task);
void scheduler_task_init(struct scheduler_task *task);
int scheduler_append(struct thread *thread);
int scheduler_kill(int32_t taskid);
int scheduler_restart(int32_t taskid);

void scheduler_enter(struct cpu_core *core);
void scheduler_switch(struct cpu_core *core, struct regs32 *regs);


void scheduler_core_idle();
