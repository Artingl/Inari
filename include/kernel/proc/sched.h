#ifndef _INARI_SCHED_H
#define _INARI_SCHED_H

#include <misc/types.h>
#include <misc/list.h>

#include <kernel/proc/proc.h>
#include <arch/paging.h>

#define SCHED_TASK_ACTIVE        0
#define SCHED_TASK_SLEEPING      1
#define SCHED_TASK_DEAD          2
#define SCHED_TASK_PAUSED        3

#define SCHED_FLAG_SYSTEM        (1 << 0)   // This flag tells scheduler that a thread is system, if it dies system will crash
#define SCHED_FLAG_SYSCALL_RSLT  (1 << 1)
#define SCHED_FLAG_SIGRETURN     (1 << 2)

struct thread;

typedef void (*thread_cleanup_t)(struct thread *, void*);

struct thread
{
    tid_t tid;
    thread_entrypoint_t entrypoint;
    pagedir_t *vmem;
    uint8_t state;
    void *sig_saved_stack;
    void *sig_saved_stack_base;
    void *saved_stack;
    uint32_t flags;
    size_t sleep_timeout;
    void *thread_stack_pointer;
    void *kernel_stack_pointer;
    thread_cleanup_t cleanup_handler;
    struct process *proc_data;

    /* Count of reschedules for this task */
    size_t reschedules_count;
    size_t cpu_time;

    uint32_t syscall_result;

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
int sched_create_thread(tid_t *tid, thread_entrypoint_t entrypoint, pagedir_t *vmem, thread_cleanup_t cleanup_handler, struct process *proc_data);
int sched_kill_thread(tid_t tid);
void sched_yield();
void sched_enter_core();
void sched_stop();
void sched_call();
int sched_thread_set_state(tid_t tid, int state);
int sched_usleep(tid_t tid, size_t us);
int sched_get_thread(tid_t tid, struct thread **task);
int sched_current_thread(tid_t *tid);
int sched_thread_set_flags(tid_t tid, uint32_t flags);

#endif