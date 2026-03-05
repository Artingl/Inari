#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/vmm.h>
#include <kernel/proc/ipc.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/sched.h>
#include <kernel/sync/spinlock.h>

#include <arch/paging.h>

#include <misc/list.h>
#include <misc/string.h>

/* TODO: What if sender processes closes while IPC handles request? IPC process might get segfault because the memory is
 * no longer available (it gets mapped from the sender's process). Solution: update vmm/pmm, allow reference counters to
 * specific memory regions, so they are deallocated only when no references are available. */

static spinlock_t lock = {0};

static ipc_handle_t handle_id = 0xff0;
static LIST_HEAD(ipc_endpoints);
static LIST_HEAD(ipc_handles);

struct ipc_endpoint {
    char name[IPC_MAX_NAME_LN];
    pid_t owner;

    int32_t current_message_id;
    struct {
        uint8_t occupied;
        pid_t source;
        ipc_handle_t ipc;
        uint32_t message;
        void *data;
        size_t data_sz;
    } messages_queue[IPC_MSG_QUEUE];

    tid_t th;
    thread_entrypoint_t handler;

    struct list_head list;
};

struct ipc_handle {
    pid_t owner;
    ipc_handle_t handle;
    uint8_t status; // 0 - not busy; 1 - busy; 2 - has answer
    int result;     // result from ipc_reply

    uint32_t message_id;
    struct ipc_endpoint *endpoint;

    struct list_head list;
};

static struct ipc_endpoint *get_endpoint_pid_thread(pid_t owner, tid_t th) {
    struct list_head *pos;
    struct ipc_endpoint *entry;

    list_for_each(pos, &ipc_endpoints) {
        entry = list_entry(pos, struct ipc_endpoint, list);
        if (entry->owner == owner && entry->th == th)
            return entry;
    }

    return NULL;
}

static struct ipc_endpoint *get_endpoint(const char *name) {
    struct list_head *pos;
    struct ipc_endpoint *entry;

    list_for_each(pos, &ipc_endpoints) {
        entry = list_entry(pos, struct ipc_endpoint, list);
        if (strcmp(entry->name, name) == 0)
            return entry;
    }

    return NULL;
}

static struct ipc_handle *get_handle(ipc_handle_t handle) {
    struct list_head *pos;
    struct ipc_handle *entry;

    list_for_each(pos, &ipc_handles) {
        entry = list_entry(pos, struct ipc_handle, list);
        if (entry->handle == handle)
            return entry;
    }

    return NULL;
}

static int cleanup_handle(struct ipc_handle *handle) {

    list_del(&handle->list);
    kfree(handle);

    return 0;
}

static int cleanup_endpoint(struct ipc_endpoint *endpoint) {
    /* Kill the IPC handler thread */
    if (endpoint->th)
        sched_kill_thread(endpoint->th);

#ifdef CONFIG_DEBUG
    kprintf("ipc: cleanup; pid %ll under %s", endpoint->owner, endpoint->name);
#endif

    /* Don't forget to cleanup handles */
    struct list_head *pos;
    struct ipc_handle *entry;

    list_for_each(pos, &ipc_handles) {
        entry = list_entry(pos, struct ipc_handle, list);
        if (entry->endpoint == endpoint)
            /* Just set it to NULL, so referencing endpoints would not cause page fault */
            entry->endpoint = NULL;
    }

    list_del(&endpoint->list);
    kfree(endpoint);

    return 0;
}

int ipc_create(struct process *proc, const char *name, thread_entrypoint_t handler) {
    int res = 0;
    uint32_t flags;
    spin_lock_irqsave(&lock, flags);

    if (get_endpoint(name)) {
        res = -IPCBSY;
        goto end;
    }

    struct ipc_endpoint *endpoint = kmalloc(sizeof(struct ipc_endpoint));
    if (!endpoint) {
        res = -ENOMEM;
        goto end;
    }

    memcpy(endpoint->name, name, strlen(name) + 1 > IPC_MAX_NAME_LN ? IPC_MAX_NAME_LN : strlen(name) + 1);
    memset(&endpoint->messages_queue, 0, sizeof(endpoint->messages_queue));
    endpoint->owner = proc->pid;
    endpoint->handler = handler;
    endpoint->current_message_id = -1;

    /* Spawn the IPC handler thread */
    if ((res = spawn_thread(&endpoint->th, endpoint->owner, handler)) != 0) {
        kfree(endpoint);
        goto end;
    }

    list_add(&endpoint->list, &ipc_endpoints);

#ifdef CONFIG_DEBUG
    kprintf("ipc: endpoint created by %ll under %s", proc->pid, name);
#endif
end:
    spin_unlock_irqrestore(&lock, flags);
    return res;
}

int ipc_free(struct process *proc, const char *name) {
    int res = 0;
    uint32_t flags;
    struct ipc_endpoint *endpoint;
    spin_lock_irqsave(&lock, flags);

    if (!(endpoint = get_endpoint(name))) {
        res = -IPCNONE;
        goto end;
    }

    if (endpoint->owner != proc->pid) {
        res = -IPCACCES;
        goto end;
    }

    res = cleanup_endpoint(endpoint);
end:
    spin_unlock_irqrestore(&lock, flags);
    return res;
}

void ipc_cleanup(struct process *proc) {
    uint32_t flags;
    spin_lock_irqsave(&lock, flags);

    struct list_head *pos, *n;
    struct ipc_endpoint *entry;

    list_for_each_safe(pos, n, &ipc_endpoints) {
        entry = list_entry(pos, struct ipc_endpoint, list);
        if (entry->owner == proc->pid)
            cleanup_endpoint(entry);
    }

    spin_unlock_irqrestore(&lock, flags);
}

void ipc_announce_death(struct process *proc, tid_t th) {
    uint32_t flags;
    spin_lock_irqsave(&lock, flags);

    struct list_head *pos;
    struct ipc_endpoint *entry;

    list_for_each(pos, &ipc_endpoints) {
        entry = list_entry(pos, struct ipc_endpoint, list);
        if (entry->th == th && entry->owner == proc->pid) {
            cleanup_endpoint(entry);
            break;
        }
    }

    spin_unlock_irqrestore(&lock, flags);
}

int ipc_fetch_next(struct process *proc, struct thread *th, pid_t *source, uint32_t *message, void **data, size_t *data_sz) {
    int res = 0;
    uint32_t id = 0;
    uint32_t flags;
    struct ipc_endpoint *endpoint;

    do {
        spin_lock_irqsave(&lock, flags);

        if (!(endpoint = get_endpoint_pid_thread(proc->pid, th->tid))) {
            res = -IPCNONE;
            goto end;
        }

        if (endpoint->owner != proc->pid) {
            res = -IPCACCES;
            goto end;
        }

        if (endpoint->current_message_id != -1) {
            id = endpoint->current_message_id;
            break;
        }

        if (endpoint->messages_queue[id].occupied)
            break;

        id++;
        if (id >= IPC_MSG_QUEUE)
            id = 0;
        spin_unlock_irqrestore(&lock, flags);
        sched_yield();
    } while (1);

    if (source)
        *source = endpoint->messages_queue[id].source;
    if (message)
        *message = endpoint->messages_queue[id].message;
    if (data_sz)
        *data_sz = endpoint->messages_queue[id].data_sz;
    if (data)
        *data = endpoint->messages_queue[id].data;
    endpoint->current_message_id = id;
end:
    spin_unlock_irqrestore(&lock, flags);
    return res;
}

int ipc_reply(struct process *proc, struct thread *th, int status) {
    int res = 0;
    uint32_t flags;
    struct ipc_endpoint *endpoint;
    struct ipc_handle *handle;
    struct process *endpoint_proc;
    spin_lock_irqsave(&lock, flags);

    if (!(endpoint = get_endpoint_pid_thread(proc->pid, th->tid))) {
        res = -IPCNONE;
        goto end;
    }

    /* Note: no need to check PID owner; get_endpoint_pid_thread ensures current process is the owner */

    if (endpoint->current_message_id == -1 || endpoint->current_message_id >= IPC_MSG_QUEUE) {
        res = -IPCNONE;
        goto end;
}

    if ((res = proc_get_process(endpoint->owner, &endpoint_proc)) != 0)
        goto end;

    /* Deallocate virtual memory from ipc_send */
    if (endpoint->messages_queue[endpoint->current_message_id].data) {
        void *data = endpoint->messages_queue[endpoint->current_message_id].data;
        size_t sz = endpoint->messages_queue[endpoint->current_message_id].data_sz;
        vmm_free_pages(endpoint_proc->descriptor.vmem, data, MAX(sz >> 12, 1));
        arch_unmap_page(endpoint_proc->descriptor.vmem, data, MAX(sz, PAGE_SIZE));
    }

    endpoint->messages_queue[endpoint->current_message_id].occupied = 0;

    /* Send answer to handle */
    if (!(handle = get_handle(endpoint->messages_queue[endpoint->current_message_id].ipc))) {
        res = -IPCNONE;
        goto end;
    }

    handle->status = 2;
    handle->result = status;
end:
    endpoint->current_message_id = -1;
    spin_unlock_irqrestore(&lock, flags);
    return res;
}

int ipc_open(struct process *proc, const char *name, ipc_handle_t *ipc) {
    int res = 0;
    uint32_t flags;
    struct ipc_endpoint *endpoint;
    spin_lock_irqsave(&lock, flags);

    if (!(endpoint = get_endpoint(name))) {
        res = -IPCNONE;
        goto end;
    }

    struct ipc_handle *handle = kmalloc(sizeof(struct ipc_handle));
    if (!handle) {
        res = -ENOMEM;
        goto end;
    }

    handle->handle = handle_id++;
    handle->endpoint = endpoint;
    handle->owner = proc->pid;

    if (ipc)
        *ipc = handle->handle;
    list_add(&handle->list, &ipc_handles);
end:
    spin_unlock_irqrestore(&lock, flags);
    return res;
}

int ipc_close(struct process *proc, ipc_handle_t ipc) {
    int res = 0;
    uint32_t flags;
    struct ipc_handle *handle;
    spin_lock_irqsave(&lock, flags);

    if (!(handle = get_handle(ipc))) {
        res = -IPCNONE;
        goto end;
    }

    if (handle->owner != proc->pid) {
        res = -IPCACCES;
        goto end;
    }

    cleanup_handle(handle);
end:
    spin_unlock_irqrestore(&lock, flags);
    return res;
}

int ipc_send(struct process *proc, ipc_handle_t ipc, uint32_t message, void *data, size_t data_sz) {
    /* Verify virtual memory integrity */
    if (data) {
        if (!VMM_IS_PTR_USERSPACE(data) || !VMM_IS_PTR_USERSPACE((uintptr_t)data + data_sz))
            return -EINVAL;

        /* Ensure alignment */
        if (((uintptr_t)data % PAGE_SIZE) != 0)
            return -EINVAL;
    }

    int res = 0;
    uint32_t flags;
    struct ipc_handle *handle;
    struct process *endpoint_proc;
    void *vbase_endpoint;
    spin_lock_irqsave(&lock, flags);

    if (!(handle = get_handle(ipc))) {
        res = -IPCNONE;
        goto end;
    }

    if (handle->owner != proc->pid) {
        res = -IPCACCES;
        goto end;
    }

    if (handle->status != 0) {
        res = -IPCBSY;
        goto end;
    }

    if ((res = proc_get_process(handle->endpoint->owner, &endpoint_proc)) != 0)
        goto end;

    handle->message_id = 0;
    while (handle->message_id < IPC_MSG_QUEUE && handle->endpoint->messages_queue[handle->message_id].occupied)
        handle->message_id++;

    /* Check if we starved the IPC queue */
    if (handle->message_id >= IPC_MSG_QUEUE) {
        res = -IPCBSY;
        goto end;
    }

    /* Map the data from source process to the endpoint process */
    if (data) {
        arch_switch_pagedir(endpoint_proc->descriptor.vmem);
        vbase_endpoint = vmm_alloc_vmem_user(endpoint_proc->descriptor.vmem, MAX(data_sz >> 12, 1));
        if (!vbase_endpoint) {
            res = -ENOMEM;
            goto end;
        }

        arch_map_page(endpoint_proc->descriptor.vmem, vbase_endpoint, arch_virt_to_phys(proc->descriptor.vmem, data),
                    MAX(data_sz, PAGE_SIZE), PAGE_RW | PAGE_PRESENT | PAGE_USR);
    }
    else vbase_endpoint = NULL;

    handle->endpoint->messages_queue[handle->message_id].source = proc->pid;
    handle->endpoint->messages_queue[handle->message_id].ipc = ipc;
    handle->endpoint->messages_queue[handle->message_id].data = vbase_endpoint;
    handle->endpoint->messages_queue[handle->message_id].data_sz = data_sz;
    handle->endpoint->messages_queue[handle->message_id].message = message;
    handle->endpoint->messages_queue[handle->message_id].occupied = 1;
    handle->status = 1;
end:
    spin_unlock_irqrestore(&lock, flags);
    return res;
}

int ipc_wait(struct process *proc, ipc_handle_t ipc, uint8_t do_sleep) {
    int res = 0;
    uint32_t flags;
    struct ipc_handle *handle;

    do {
        spin_lock_irqsave(&lock, flags);

        if (!(handle = get_handle(ipc))) {
            res = -IPCNONE;
            goto end;
        }

        if (handle->owner != proc->pid) {
            res = -IPCACCES;
            goto end;
        }

        if (handle->status != 1) {
            handle->status = 0;
            res = handle->result;
            break;
        }

        spin_unlock_irqrestore(&lock, flags);
        if (do_sleep)
            sched_yield();
    } while (do_sleep);

end:
    spin_unlock_irqrestore(&lock, flags);
    return res;
}
