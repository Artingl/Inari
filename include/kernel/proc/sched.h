#ifndef _INARI_SCHED_H
#define _INARI_SCHED_H

#include <misc/types.h>
#include <misc/list.h>

#include <kernel/proc/proc.h>
#include <kernel/mm/page.h>

#define SCHED_TASK_ACTIVE   0
#define SCHED_TASK_SLEEPING 1
#define SCHED_TASK_DEAD     2

#define SCHED_FLAG_SYSTEM (1 << 0)   // This flag tells scheduler that a thread is system, if it dies system will crash

typedef void (*task_entrypoint_t)();
typedef void (*task_signal_t)(uint32_t signo);

struct thread
{
    tid_t tid;
    task_entrypoint_t entrypoint;
    task_signal_t signal_handler;
    pagedir_t vmem;
    uint8_t inside_signal;
    uint8_t state;
    uint32_t saved_sp;
    uint32_t flags;
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
    struct thread *task;
};

void sched_thread_preentry();

int sched_init();
int sched_is_running();
int sched_create_thread(tid_t *tid, task_entrypoint_t entrypoint, pagedir_t vmem);
int sched_signal_thread(tid_t tid, uint32_t signo);
void sched_yield();
void sched_enter_core();
void sched_stop();
void sched_call();
void sched_usleep(tid_t tid, size_t us);
int sched_get_thread(tid_t tid, struct thread **task);
int sched_current_thread(tid_t *tid);
int sched_thread_set_flags(tid_t tid, uint32_t flags);

#endif