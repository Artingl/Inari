#ifndef _INARI_PROC_H
#define _INARI_PROC_H

#include <kernel/mm/page.h>

#include <misc/types.h>
#include <misc/list.h>

typedef void (*thread_entrypoint_t)();
typedef uint64_t tid_t; // Thread ID
typedef uint64_t pid_t; // Process ID

typedef struct
{
    pagedir_t vmem;
    void *entrypoint;
} task_descriptor_t;

struct process
{
    pid_t pid;
    task_descriptor_t descriptor;

    /* TODO: should there be a limit? */
    tid_t threads[CONFIG_PROC_MAX_THREADS];

    struct list_head list;
};

int proc_init();

int exit(pid_t pid, int exit_code);
int execp(pid_t *pid, const char *path);

int kill_process(pid_t pid);
int spawn_process(pid_t *pid, task_descriptor_t descriptor);
int spawn_thread(tid_t *tid, pid_t pid, thread_entrypoint_t entrypoint);

#endif