#include <kernel/printk.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/sched.h>
#include <kernel/proc/pe.h>
#include <kernel/sync/spinlock.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/sys/vfs.h>
#include <kernel/errno.h>

#include <misc/string.h>
#include <misc/list.h>

static spinlock_t lock;
static pid_t last_pid = 1;

static LIST_HEAD(processes);

struct process
{
    pid_t pid;
    task_descriptor_t descriptor;

    /* TODO: should there be a limit? */
    tid_t threads[CONFIG_PROC_MAX_THREADS];

    struct list_head list;
};

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

int proc_init()
{
    spinlock_init(&lock);
    return 0;
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
    if ((res = pe_load(vmem, &entrypoint, buf, size)) != 0)
    {
        page_dealloc_dir(vmem);
        goto dealloc;
    }

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
    if (!pid) return -EINVAL;

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
    *pid = proc->pid;
    spin_unlock_irqrestore(&lock, flags);

    spawn_thread(&tid, *pid, descriptor);
    return 0;
}

int kill_process(pid_t pid)
{
    /* Kill all threads and kfree the process struct */
    return -ESRCH;
}

int spawn_thread(tid_t *tid, pid_t pid, task_descriptor_t descriptor)
{
    if (!tid) return -EINVAL;

    size_t i;
    int res = 0;
    struct process *proc;
    uint32_t flags;

    spin_lock_irqsave(&lock, flags);
    if ((res = get_process(&proc, pid)) != 0)
        goto end;
    if ((res = sched_create_thread(tid, descriptor.entrypoint, descriptor.vmem)) != 0)
        goto end;
    if (pid == 1)
        sched_thread_set_flags(*tid, SCHED_FLAG_SYSTEM);

    for (i = 0; i < CONFIG_PROC_MAX_THREADS; i++)
        if (proc->threads[i] == 0)
        {
            proc->threads[i] = *tid;
            break;
        }
end:
    spin_unlock_irqrestore(&lock, flags);
    return res;
}
