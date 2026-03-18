#ifndef _INARI_PROC_H
#define _INARI_PROC_H

#include <arch/paging.h>

#include <misc/list.h>
#include <misc/types.h>

#define PROC_ARGS_BASE    0x1900000 // Address where process arguments are location in a form of char **
#define PROC_OPTIONS_BASE 0x1902000 // Address where pointer to options linked list is located

#define EXEC_FLAG_CP_OPTIONS (1 << 0) // Copies options from the caller process

typedef void (*thread_entrypoint_t)(void *);
typedef void (*proc_signal_t)(uint32_t);
typedef uint64_t tid_t; // Thread ID
typedef uint64_t pid_t; // Process ID

typedef struct {
    pagedir_t *vmem;
    void *entrypoint;
} task_descriptor_t;

union process_option_value {
    char value[256];
    uint32_t u32;
    uint64_t u64;
    size_t sz;
} __attribute__((packed));

struct process_option {
    char name[32];
    union process_option_value value;
    struct process_option *next;
    struct process_option *prev;
} __attribute__((packed));

struct process {
    pid_t pid;
    int exit_code;

    char path[CONFIG_VFS_NAME_MAX];

    uint32_t pending_signal;
    proc_signal_t signal_handler[32];

    task_descriptor_t descriptor;

    /* TODO: should there be a limit? */
    tid_t threads[CONFIG_PROC_MAX_THREADS];

    struct process_option *options_list;

    struct list_head handles;

    struct list_head list;
};

struct process_waitpid {
    pid_t pid;          // The proccess the thread is waiting for
    tid_t observer_tid; // The thread that currently sleeps

    struct list_head list;
};

int proc_init();

int proc_add_option(pid_t pid, const char *name, union process_option_value value);
int proc_get_option(pid_t pid, const char *name, union process_option_value *result);
int proc_free_option(pid_t pid, const char *name);

int exit(pid_t pid, int exit_code);
int execp(pid_t *pid, const char *path);
int execpv(pid_t *pid, const char *path, int argc, char **argv);
int execpvf(pid_t *pid, const char *path, int flags, int argc, char **argv);
int proc_get_process(pid_t pid, struct process **proc);
int proc_install_signal(pid_t pid, proc_signal_t handler, uint32_t signo);
int proc_signal(pid_t pid, uint32_t signo);
int proc_ls(int idx, char *name, pid_t *pid, double *usg);
int kill_process(pid_t pid, int exit_code);
int spawn_process(pid_t *pid, int flags, const char *path, task_descriptor_t descriptor);
int spawn_thread(tid_t *tid, pid_t pid, thread_entrypoint_t entrypoint);
int waitpid(pid_t pid, tid_t observer_tid);

#endif
