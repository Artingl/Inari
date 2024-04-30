#pragma once

#include <kernel/lock/spinlock.h>

#define THREAD_ACTIVE   0x00
#define THREAD_INACTIVE 0x01
#define THREAD_SLEEPING 0x02

struct thread;
typedef struct thread thread_t;
typedef void(*thread_entry_t)(thread_t*, void*);

struct thread {
    void *data;
    thread_entry_t entry;

    spinlock_t lock;
    int32_t tid; // thread id

    int32_t state;
    int32_t sleep_until;
};

int thread_init(struct thread *thread, void *data, thread_entry_t entry);
int thread_start(struct thread *thread);
int thread_kill(struct thread *thread, int code);
thread_t *thread_self();


void thread_entrypoint();
