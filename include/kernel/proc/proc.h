#ifndef _INARI_PROC_H
#define _INARI_PROC_H

#include <kernel/mm/page.h>

#include <misc/types.h>
#include <misc/list.h>

#define PROC_ARGS_BASE  0x900000

typedef void (*thread_entrypoint_t)(void*);
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
    int exit_code;
    task_descriptor_t descriptor;

    /* TODO: should there be a limit? */
    tid_t threads[CONFIG_PROC_MAX_THREADS];

    struct list_head list;
};

struct process_waitpid
{
    pid_t pid;          // The proccess the thread is waiting for 
    tid_t observer_tid; // The thread that currently sleeps

    struct list_head list;
};

int proc_init();

int exit(pid_t pid, int exit_code);
int execp(pid_t *pid, const char *path);
int execpv(pid_t *pid, const char *path, int argc, char **argv);

int kill_process(pid_t pid, int exit_code);
int spawn_process(pid_t *pid, task_descriptor_t descriptor);
int spawn_thread(tid_t *tid, pid_t pid, thread_entrypoint_t entrypoint);
int waitpid(pid_t pid, tid_t observer_tid);

#endif