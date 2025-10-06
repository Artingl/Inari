#ifndef _INARI_SCHED_H
#define _INARI_SCHED_H

#include <misc/types.h>
#include <misc/list.h>

#define SCHED_TASK_ACTIVE   0
#define SCHED_TASK_SLEEPING 1
#define SCHED_TASK_DEAD     2

typedef size_t tid_t;
typedef void (*task_entrypoint_t)();
typedef void (*task_signal_t)(uint32_t signo);

struct sched_task
{
    tid_t task_id;
    task_entrypoint_t entrypoint;
    task_signal_t signal_handler;
    uint8_t inside_signal;
    uint8_t state;
    uint32_t saved_sp;
    size_t sleep_timeout;
    void *stack_pointer;

    /* Count of reschedules for this task */
    size_t reschedules_count;
    size_t cpu_time;

    struct list_head list;
};

struct sched_core
{
    uint8_t active;
    uint32_t core_id;
    size_t last_schedule_ticks;
    struct sched_task *task;
};

extern void sched_task_preentry();

extern int sched_init();
extern int sched_is_running();
extern int sched_add_task(tid_t *tid, task_entrypoint_t entrypoint);
extern int sched_signal_task(tid_t tid, uint32_t signo);
extern void sched_yield();
extern void sched_enter_core();
extern void sched_stop();
extern void sched_call();
extern void sched_usleep(tid_t tid, size_t us);
extern int sched_get_task(tid_t task_id, struct sched_task **task);
extern int sched_current_task(tid_t *tid);

#endif