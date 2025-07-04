#include <kernel/core/module/module.h>
#include <kernel/libc/string.h>
#include <kernel/driver/memory/memory.h>
#include <kernel/core/errno.h>
#include <kernel/core/syscall/syscall.h>

dynlist_t modules_list = {0};
spinlock_t modules_list_lock = {0};
uint8_t disable_modules_service = 0;

void console_load_module();

static void module_handle_events(struct kern_module *module)
{
    size_t i;
    struct kern_module_event *event;
    spinlock_acquire(&module->events_lock);
    if (dynlist_size(module->events_pool) == 0) goto end;

    for (i = 0; i < dynlist_size(module->events_pool); i++)
    {
        event = dynlist_get(module->events_pool, i, struct kern_module_event*);
        if (event != NULL)
        {
            module->event_handler(event->event_id, event->payload);

            if (event->event_id == MODULE_EVENT_UNLOAD)
            {
                module->is_active = 0;
            }

            // Do not forget to deallocate the event struct memory!
            kfree(event);
        }
    }

    dynlist_clear(module->events_pool);
end:
    spinlock_release(&module->events_lock);
}

static void module_entrypoint(struct kern_module *module)
{
    int code = module->entrypoint();
    if (code != K_OKAY)
    {
        printk("module: unable to load module %s, code: %d", module->name, code);
        kern_module_unload(module->name);
        return;
    }

    printk("module: loaded module %s", (char*)&module->name[0]);
}

static void module_events_entrypoint(struct kern_module *module)
{
    while (module->is_active)
    {
        module_handle_events(module);
        scheduler_yield();
    }

    kfree(module);
    printk("module: unloaded %s", module->name);
}

void kern_modules_init()
{
    printk("module: loading all kernel modules");

    // Load all modules
    console_load_module();
}

__attribute__ ((optimize(0))) void kern_modules_cleanup()
{
    size_t i, active_modules = 0;
    struct kern_module *module, *found_module = NULL;

    printk("module: unloading all modules");
    spinlock_acquire(&modules_list_lock);
    disable_modules_service = 1;

    // Send unload event to all modules
    for (i = 0; i < dynlist_size(modules_list); i++)
    {
        module = dynlist_get(modules_list, i, struct kern_module*);
        if (module != NULL)
        {
            struct kern_module_event *event = kmalloc(sizeof(struct kern_module_event));
            event->event_id = MODULE_EVENT_UNLOAD;
            event->payload = NULL;
            dynlist_append(module->events_pool, event);

            printk("module: unloading %s", module->name);
            do
            { } while (module->is_active);

            scheduler_kill_task(module->entry_tid);
            scheduler_kill_task(module->events_tid);
        }
    }

    printk("module: done unloading");
    spinlock_release(&modules_list_lock);
}

// This function does NOT acquire the spinlock, be careful!
static struct kern_module *kern_find_module(char *name)
{
    size_t i;
    struct kern_module *module, *found_module = NULL;

    for (i = 0; i < dynlist_size(modules_list); i++)
    {
        module = dynlist_get(modules_list, i, struct kern_module*);
        if (module != NULL && strcmp((char*)module->name, name) == 0 && module->is_active)
        {
            found_module = module;
            goto end;
        }
    }
    
end:
    return found_module;
}

void kern_module_load(char *name, module_entrypoint_t *entrypoint, module_event_handler_t *event_handler)
{
    if (disable_modules_service == 1) return;

    spinlock_acquire(&modules_list_lock);
    if (kern_find_module(name) != NULL)
    {
        printk("module: unable to load duplicate %s module", name);
        return;
    }
    
    struct kern_module *module = kmalloc(sizeof(struct kern_module));
    memset(module, 0, sizeof(struct sched_task));

    strcpy((char*)&module->name[0], name);
    module->is_active = 1;
    module->entrypoint = entrypoint;
    module->event_handler = event_handler;
    module->entry_tid = scheduler_create_task(&module_entrypoint, module);
    module->events_tid = scheduler_create_task(&module_events_entrypoint, module);
    dynlist_append(modules_list, module);
end:
    spinlock_release(&modules_list_lock);
}

void kern_module_unload(char * name)
{
    spinlock_acquire(&modules_list_lock);
    size_t i;
    struct kern_module *found_module, *module = kern_find_module(name);
 
    if (module == NULL) goto end;
    if (module->is_active != 1) return;

    // Kill the entry task, but do not kill the events task
    // because it will handle the UNLOAD event
    scheduler_kill_task(module->entry_tid);

    // Remove the module from the list
    for (i = 0; i < dynlist_size(modules_list); i++)
    {
        found_module = dynlist_get(modules_list, i, struct kern_module*);
        if (found_module != NULL && strcmp((char*)found_module->name, name) == 0)
        {
            dynlist_remove(modules_list, i);
            break;
        }
    }

    spinlock_acquire(&module->events_lock);
    struct kern_module_event *event = kmalloc(sizeof(struct kern_module_event));
    event->event_id = MODULE_EVENT_UNLOAD;
    event->payload = NULL;
    dynlist_append(module->events_pool, event);
    spinlock_release(&module->events_lock);
end:
    spinlock_release(&modules_list_lock);
}

void kern_module_send_event(char *name, uint32_t event_id, void *payload)
{
    spinlock_acquire(&modules_list_lock);
    struct kern_module *module = kern_find_module(name);
    if (module == NULL) goto end;
    if (module->is_active != 1) return;
    spinlock_acquire(&module->events_lock);

    struct kern_module_event *event = kmalloc(sizeof(struct kern_module_event));
    event->event_id = event_id;
    event->payload = payload;
    dynlist_append(module->events_pool, event);
    spinlock_release(&module->events_lock);
end:
    spinlock_release(&modules_list_lock);
}