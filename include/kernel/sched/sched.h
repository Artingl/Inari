#ifndef _INARI_SCHED_H
#define _INARI_SCHED_H

#include <misc/types.h>
#include <misc/list.h>

#define SCHED_IDLE_TASK_ID         1

#define SCHED_TASK_ACTIVE   0
#define SCHED_TASK_SLEEPING 1
#define SCHED_TASK_DEAD     2

#define SCHED_SIGNAL_KILL   0 // Tells the scheduler to kill the task gracefully
#define SCHED_SIGNAL_TERM   1 // Kills immediately the task

typedef size_t tid_t;
typedef void (*task_entrypoint_t)();

extern int sched_init();
extern int sched_add_task(tid_t *tid, task_entrypoint_t entrypoint);
extern int sched_remove_task(tid_t tid);
extern int sched_signal_task(tid_t tid, uint32_t signal);
extern void sched_yield();
extern void sched_enter_core();
extern void sched_stop();
extern void sched_call();
extern tid_t sched_current_task();

#endif