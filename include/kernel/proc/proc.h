#ifndef _INARI_PROC_H
#define _INARI_PROC_H

#include <kernel/mm/page.h>

#include <misc/types.h>

typedef uint64_t tid_t; // Thread ID
typedef uint64_t pid_t; // Process ID

typedef struct
{
    pagedir_t vmem;
    void *entrypoint;
} task_descriptor_t;

int proc_init();

int execp(pid_t *pid, const char *path);

int kill_process(pid_t pid);
int spawn_process(pid_t *pid, task_descriptor_t descriptor);
int spawn_thread(tid_t *tid, pid_t pid, task_descriptor_t descriptor);

#endif