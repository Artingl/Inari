#ifndef _INARI_MUTEX_H
#define _INARI_MUTEX_H

#include <misc/types.h>

#include <stdatomic.h>

typedef struct mutex {
    atomic_flag lock;
} mutex_t;

extern void mutex_init(mutex_t *lock);
extern void mutex_lock(mutex_t *lock);
extern int mutex_test(mutex_t *lock);
extern void mutex_unlock(mutex_t *lock);

#endif