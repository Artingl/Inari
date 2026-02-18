#include <kernel/printk.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/sched.h>
#include <kernel/proc/pe.h>
#include <kernel/proc/signals.h>
#include <kernel/sync/spinlock.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/vmm.h>
#include <kernel/sys/vfs.h>
#include <kernel/errno.h>

#include <misc/string.h>
#include <misc/list.h>

static spinlock_t lock;
static pid_t last_pid = 1;

static LIST_HEAD(processes);

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

static void thread_cleanup(struct thread *th, struct process *proc)
{
    if (!proc) return;

    size_t i, total_threads = 0;
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
        list_del(&proc->list);
        /* TODO: cleanup process's virtual memory! */
    }
}

int proc_init()
{
    spinlock_init(&lock);
    return 0;
}

int exit(pid_t pid, int exit_code)
{
    /* TODO: exit code has no actual use for now */
    return kill_process(pid);
}

void test()
{
    printk("!!!");
}

int execp(pid_t *pid, const char *path)
{
    pagedir_t vmem;
    size_t size;
    vfs_handle_t hndl;
    void *entrypoint = NULL;
    uint8_t *buf;
    int res = 0;

    if ((res = vfs_open(&hndl, path, VFS_READ)) != 0)
        goto end;
    if ((res = vfs_size(hndl, &size)) != 0)
        goto close_f;
    if ((buf = (uint8_t*)kmalloc(size)) == NULL)
    {
        res = -ENOMEM;
        goto close_f;
    }

    if ((res = vfs_read(hndl, (void*)buf, size, NULL)) != 0)
        goto dealloc;

    vmem = page_alloc_dir();
    page_in_kernel_glbl(0);
    if ((res = pe_load(vmem, &entrypoint, buf, size)) != 0)
    {
        page_in_kernel_glbl(1);
        page_dealloc_dir(vmem);
        goto dealloc;
    }
    page_in_kernel_glbl(1);

    spawn_process(pid, (task_descriptor_t){ .entrypoint = entrypoint, .vmem = vmem });
dealloc:
    kfree((void*)buf);
close_f:
    vfs_close(hndl);
end:
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
    proc->descriptor = descriptor;
    memset((void*)&proc->threads[0], 0, sizeof(proc->threads));
    list_add(&proc->list, &processes);
    if (pid)
        *pid = proc->pid;
    spin_unlock_irqrestore(&lock, flags);

    spawn_thread(&tid, proc->pid, (thread_entrypoint_t)descriptor.entrypoint);
    return 0;
}

int kill_process(pid_t pid)
{
    int res = 0;
    size_t i;
    uint32_t flags;
    struct process *proc;
    spin_lock_irqsave(&lock, flags);
    if ((res = get_process(&proc, pid)) != 0)
        goto end;

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
