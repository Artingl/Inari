#pragma once

#include <kernel/libc/typedefs.h>
#include <kernel/core/lock/spinlock.h>

#define dynlist(name) dynlist_t name = {0}

typedef struct dynlist
{
    uintptr_t *items;
    size_t size;
    spinlock_t lock;
} dynlist_t;

#define dynlist_append(list, data) (__dynlist_append(&(list), (uintptr_t)(data)))
#define dynlist_pop(list, type) ((type)__dynlist_pop(&(list)))
#define dynlist_get(list, idx, type) ((type)__dynlist_get(&(list), idx))
#define dynlist_set(list, idx, data) (__dynlist_set(&(list), idx, (uintptr_t)data))
#define dynlist_remove(list, idx) __dynlist_remove(&(list), idx)
#define dynlist_size(list) (list.size)
#define dynlist_is_empty(list) (dynlist_size(list) == 0)
#define dynlist_insert(list, idx, data) (__dynlist_insert(&(list), idx, (uintptr_t)data))

uintptr_t __dynlist_get(dynlist_t *list, int idx);
uintptr_t __dynlist_pop(dynlist_t *list);
int __dynlist_set(dynlist_t *list, int idx, uintptr_t data);
int __dynlist_insert(dynlist_t *list, int idx, uintptr_t data);
int __dynlist_append(dynlist_t *list, uintptr_t data);
int __dynlist_remove(dynlist_t *list, int idx);
