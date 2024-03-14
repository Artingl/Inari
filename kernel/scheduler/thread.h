#pragma once

#include <kernel/lock/spinlock.h>

typedef void(*thread_entry_t)(void*);

#define THREAD_ACTIVE   0x00
#define THREAD_INACTIVE 0x01
#define THREAD_SLEEPING 0x02

typedef struct thread {
    void *data;
    thread_entry_t entry;

    spinlock_t lock;
    int32_t tid; // thread id

    int32_t state;
    int32_t sleep_until;
} thread_t;

int thread_init(struct thread *thread, void *data, thread_entry_t entry);
int thread_start(struct thread *thread);
int thread_kill(struct thread *thread, int code);
thread_t *thread_self();
