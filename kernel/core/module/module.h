#pragma once

#define MODULE_INIT(init)

#include <kernel/list/dynlist.h>
#include <kernel/core/sched/scheduler.h>
#include <kernel/libc/typedefs.h>
#include <kernel/core/lock/spinlock.h>

#define KERN_MODULE_NAMELEN 0xff

#define MODULE_EVENT_UNLOAD 0x00

typedef int(module_entrypoint_t)();
typedef int(module_event_handler_t)(uint32_t event_id, void *payload);

struct kern_module
{
    char name[KERN_MODULE_NAMELEN];
    dynlist_t events_pool;
    spinlock_t events_lock;
    uint8_t is_active;
    module_entrypoint_t *entrypoint;
    module_event_handler_t *event_handler;
    tid_t entry_tid;
    tid_t events_tid;
};

struct kern_module_event
{
    uint32_t event_id;
    void *payload;
};

void kern_modules_cleanup();
void kern_modules_init();

void kern_module_load(char *name, module_entrypoint_t *entrypoint, module_event_handler_t *event_handler);
void kern_module_unload(char *name);
void kern_module_send_event(char *name, uint32_t event_id, void *payload);
