#include <kernel/printk.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/sched.h>
#include <kernel/proc/pe.h>
#include <kernel/proc/signals.h>
#include <kernel/sync/spinlock.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/pmm.h>
#include <kernel/sys/vfs.h>
#include <kernel/errno.h>

#include <misc/string.h>
#include <misc/list.h>

static spinlock_t lock;
static pid_t last_pid = 1;

static LIST_HEAD(processes);
static LIST_HEAD(waitpid_threads);

static inline void copy_to_fixed_buffer(int count, char **src_argv, void *buffer) {
    char **new_argv = (char **)buffer;
    char *data_ptr = (char *)buffer + ((count + 1) * sizeof(char *));
    for (int i = 0; i < count; i++) {
        new_argv[i] = data_ptr;
        strcpy(data_ptr, src_argv[i]);
        data_ptr += strlen(src_argv[i]) + 1;
    }
    new_argv[count] = NULL;
}

static inline void copy_to_fixed_buffer_witharg(int count, char **src_argv, const char *path, void *buffer) {
    char **new_argv = (char **)buffer;
    
    /* Determine the actual number of elements we will have */
    /* If path exists, we have count + 1 elements. */
    int has_path = (path != NULL) ? 1 : 0;
    int total_count = (src_argv && count > 0) ? (count + has_path) : has_path;

    /* data_ptr starts after the pointer array (including the NULL terminator) */
    char *data_ptr = (char *)buffer + ((total_count + 1) * sizeof(char *));
    int current_idx = 0;

    /* Add Path as the first argument if it exists */
    if (has_path) {
        new_argv[current_idx] = data_ptr;
        strcpy(data_ptr, path);
        data_ptr += strlen(path) + 1;
        current_idx++;
    }

    /* Append original argv elements if they exist */
    if (src_argv && count > 0) {
        for (int i = 0; i < count; i++) {
            new_argv[current_idx] = data_ptr;
            strcpy(data_ptr, src_argv[i]);
            data_ptr += strlen(src_argv[i]) + 1;
            current_idx++;
        }
    }
    new_argv[total_count] = NULL;
}

static int get_process(struct process **proc, pid_t pid)
{
    struct list_head *pos;
    struct process *entry;

    list_for_each(pos, &processes) {
        entry = list_entry(pos, struct process, list);
        if (entry->pid == pid)
        {
            *proc = entry;
            return 0;
        }
    }

    return -ESRCH;
}

struct thread *__sched_get_thread(tid_t tid);

static void thread_cleanup(struct thread *th, struct process *proc)
{
    if (!proc) return;

    struct list_head *pos;
    struct process_waitpid *entry;
    struct thread *observer_th;
    size_t i, total_threads = 0;

    uint32_t flags;
    spin_lock_irqsave(&lock, flags);

    for (i = 0; i < CONFIG_PROC_MAX_THREADS; i++)
    {
        if (proc->threads[i] == th->tid)
            proc->threads[i] = 0;
        if (proc->threads[i] != 0) total_threads++;
    }
    
    /* If we don't have any threads running, that means either they died
     * on their own or kill_process killed them. We can cleanup the process now. */
    if (total_threads == 0)
    {
        /* Check if there are any threads waiting for this process to die */
        list_for_each(pos, &waitpid_threads) {
            entry = list_entry(pos, struct process_waitpid, list);
            if (entry->pid == proc->pid)
            {
                if ((observer_th = __sched_get_thread(entry->observer_tid)) != NULL)
                {
                    observer_th->state = SCHED_TASK_ACTIVE;
                    /* Forward the exitcode value to waitpid result */
                    observer_th->syscall_result = proc->exit_code;
                    observer_th->flags |= SCHED_FLAG_SYSCALL_RSLT;
                }
                list_del(&entry->list);
                kfree(entry);
                break;
            }
        }

        arch_free_pagedir(proc->descriptor.vmem);
        list_del(&proc->list);
        /* TODO: cleanup process's virtual memory! */
    }

    spin_unlock_irqrestore(&lock, flags);
}

int proc_init()
{
    spinlock_init(&lock);
    return 0;
}

int exit(pid_t pid, int exit_code)
{
    return kill_process(pid, exit_code);
}

int execp(pid_t *pid, const char *path)
{
    execpv(pid, path, 0, NULL);
}

int execpv(pid_t *pid, const char *path, int argc, char **argv)
{
    pagedir_t *vmem = NULL;
    size_t size;
    vfs_handle_t hndl;
    void *entrypoint = NULL, *args_pbase = NULL, *tmp_args_base = NULL;
    uint8_t *buf = NULL;
    int res = 0;
    uint32_t flags;

    if (!path) return -EINVAL;

    spin_lock_irqsave(&lock, flags);

    /* Fork kernel directory for the new process */
    vmem = arch_fork_pagedir();

    /* Load process data into memory */
    if ((res = vfs_open(&hndl, path, VFS_READ)) != 0)
        goto err;
    if ((res = vfs_size(hndl, &size)) != 0)
        goto err;
    if ((buf = (uint8_t*)kmalloc(size)) == NULL || (tmp_args_base = (void*)kmalloc(PAGE_SIZE * 2)) == NULL)
        { res = -ENOMEM; goto err; }
    if ((res = vfs_read(hndl, (void*)buf, size, NULL)) != 0)
        goto err;

    /* Copy the provided arguments to temporary address to later copy it to new process memory */
    /* TODO: what if argv is larger than tmp_args_base; env vars */
    if (argc < 0) argc = 0;
    copy_to_fixed_buffer_witharg(argc, argv, path, tmp_args_base);
    arch_switch_pagedir(vmem);

    /* TODO: args size must not be limited to 8kb */
    args_pbase = pmm_alloc_pages(2);
    if (arch_map_page(
        vmem,
        (void*)PROC_ARGS_BASE,
        args_pbase,
        PAGE_SIZE * 2,
        PAGE_PRESENT | PAGE_RW | PAGE_USR) == NULL)
    {
        goto err;
    }
    vmm_disable_region(vmem, (struct reserved_memory){
        .start = PROC_ARGS_BASE,
        .end = PROC_ARGS_BASE + PAGE_SIZE * 2 });
    copy_to_fixed_buffer(argc + 1, (char**)tmp_args_base, (void*)PROC_ARGS_BASE);

    /* Load the PE into non-kernel memory */
    if ((res = pe_load(vmem, &entrypoint, buf, size)) != 0)
        goto err;

    arch_switch_pagedir(arch_get_kernel_pagedir());
    
    spin_unlock_irqrestore(&lock, flags);

    spawn_process(pid, (task_descriptor_t){ 
        .entrypoint = entrypoint, 
        .vmem = vmem
    });
    
    goto end;
err:
    arch_switch_pagedir(arch_get_kernel_pagedir());
    if (args_pbase) pmm_free_pages(args_pbase, 2);
    if (vmem) arch_free_pagedir(vmem);
    spin_unlock_irqrestore(&lock, flags);
end:
    if (tmp_args_base) kfree((void*)tmp_args_base);
    if (buf) kfree((void*)buf);
    if (hndl) vfs_close(hndl);
    return res;
}

int spawn_process(pid_t *pid, task_descriptor_t descriptor)
{
    uint32_t flags;
    struct process *proc;
    tid_t tid;

    proc = (struct process*)kmalloc(sizeof(struct process));
    if (!proc) return -ENOMEM;

    spin_lock_irqsave(&lock, flags);
    proc->pid = last_pid++;
    proc->exit_code = 0;
    proc->descriptor = descriptor;
    memset((void*)&proc->threads[0], 0, sizeof(proc->threads));
    list_add(&proc->list, &processes);
    if (pid)
        *pid = proc->pid;
    spin_unlock_irqrestore(&lock, flags);

    spawn_thread(&tid, proc->pid, (thread_entrypoint_t)descriptor.entrypoint);
    return 0;
}

int kill_process(pid_t pid, int exit_code)
{
    int res = 0;
    size_t i;
    uint32_t flags;
    struct process *proc;
    spin_lock_irqsave(&lock, flags);
    if ((res = get_process(&proc, pid)) != 0)
        goto end;
    proc->exit_code = exit_code;

    /* Kill all threads */
    for (i = 0; i < CONFIG_PROC_MAX_THREADS; i++)
        if (proc->threads[i] != 0)
        {
            /* NOTE: SIGKILL will kill a thread immediately */
            sched_signal_thread(proc->threads[i], SIGKILL);
        }
end:
    spin_unlock_irqrestore(&lock, flags);
    return res;
}

int spawn_thread(tid_t *tid, pid_t pid, thread_entrypoint_t entrypoint)
{
    size_t i;
    tid_t i_tid;
    int res = 0;
    struct process *proc;
    uint32_t flags;

    spin_lock_irqsave(&lock, flags);
    if ((res = get_process(&proc, pid)) != 0)
        goto end;
    if ((res = sched_create_thread(&i_tid, entrypoint, proc->descriptor.vmem, (void*)&thread_cleanup, (void*)proc)) != 0)
        goto end;
    if (pid == 1)
        sched_thread_set_flags(i_tid, SCHED_FLAG_SYSTEM);

    for (i = 0; i < CONFIG_PROC_MAX_THREADS; i++)
        if (proc->threads[i] == 0)
        {
            proc->threads[i] = i_tid;
            break;
        }
    if (tid)
        *tid = i_tid;
end:
    spin_unlock_irqrestore(&lock, flags);
    return res;
}

int waitpid(pid_t pid, tid_t observer_tid)
{
    int res = 0;
    uint32_t flags;
    struct process *proc;
    struct thread *observer;
    struct thread *th;
    struct process_waitpid *wait = NULL;
    
    spin_lock_irqsave(&lock, flags);
    if ((wait = (struct process_waitpid*)kmalloc(sizeof(struct process_waitpid))) == NULL)
    {
        res = -ENOMEM;
        goto end;
    }
    if ((res = get_process(&proc, pid)) != 0)
        goto err;
    if ((th = __sched_get_thread(observer_tid)) == NULL)
        goto err;

    wait->pid = pid;
    wait->observer_tid = observer_tid;
    th->state = SCHED_TASK_PAUSED;
    list_add(&wait->list, &waitpid_threads);
    goto end;
err:
    if (wait) kfree(wait);
end:
    spin_unlock_irqrestore(&lock, flags);
    return res;
}
