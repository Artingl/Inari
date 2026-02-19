#include <kernel/inari.h>
#include <kernel/printk.h>
#include <kernel/errno.h>
#include <kernel/proc/sched.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/signals.h>
#include <kernel/sys/vfs.h>
#include <kernel/sys/syscall.h>
#include <kernel/mm/page.h>
#include <kernel/console/console.h>


int syscall_handle(
    uint32_t id,
    void *param0,
    void *param1,
    void *param2,
    void *param3,
    void *param4)
{
    tid_t tid;
    char *s;
    int res = 0;
    struct thread *th;
    if (sched_current_thread(&tid) != 0)
        return -ESRCH;
    if (sched_get_thread(tid, &th) != 0)
        return -ESRCH;
    if (!tid || !th->proc_data)
        return -ESRCH;
    struct process *proc = (struct process*)th->proc_data;
    page_switch_dir(proc->descriptor.vmem);
    th->flags |= SCHED_FLAG_KERNEL_ACCESS;
    
    switch (id)
    {
    case SYSCALL_EXIT:
        res = exit(proc->pid, (int)param0);
        break;
    
    case SYSCALL_USLEEP:
        sched_usleep(tid, (size_t)param0);
        break;

    case SYSCALL_OPEN:
        res = vfs_open((vfs_handle_t*)param0, (const char*)param1, (int)param2);
        break;

    case SYSCALL_CLOSE:
        res = vfs_close((vfs_handle_t)param0);
        break;

    case SYSCALL_EXECP:
        res = execp((pid_t*)param0, (const char*)param1);
        break;
    
    case SYSCALL_READ:
        res = vfs_read((vfs_handle_t)param0, (void*)param1, (size_t)param2, (size_t*)param3);
        break;
        
    case SYSCALL_SEEK:
        res = vfs_seek((vfs_handle_t)param0, (size_t)param1);
        break;
        
    case SYSCALL_TELL:
        res = vfs_tell((vfs_handle_t)param0, (size_t*)param1);
        break;
        
    case SYSCALL_SIZE:
        res = vfs_size((vfs_handle_t)param0, (size_t*)param1);
        break;
    
    case SYSCALL_GET_PID:
        if (param0)
            *((pid_t*)param0) = proc->pid;
        break;

    case SYSCALL_SPAWN_THREAD:
        return spawn_thread((tid_t*)param0, (pid_t)param1, (thread_entrypoint_t)param2);
    
    case SYSCALL_GET_TID:
        if (param0)
            *((pid_t*)param0) = th->tid;
        break;

    case SYSCALL_KILL_THREAD:
        /* NOTE: SIGKILL will kill a thread immediately */
        res = sched_signal_thread((tid_t)param0, SIGKILL);
        break;

    case SYSCALL_MOUNT:
        res = vfs_mount((dev_t)param0, (const char*)param1);
        break;

    case SYSCALL_UNMOUNT:
        res = vfs_unmount((const char*)param1);
        break;
    
    case SYSCALL_READDIR:
        res = vfs_readdir((const char*)param0, (struct vfs_node*)param1);
        break;
    
    case SYSCALL_WRITE:
        res = vfs_write((vfs_handle_t)param0, (void*)param1, (size_t)param2);
        break;
    
    case SYSCALL_WAITPID:
        res = waitpid((pid_t)param0, th->tid);
        break;

    case SYSCALL_EXECPV:
        res = execpv((pid_t*)param0, (const char*)param1, (int)param2, (char**)param3);
        break;

    case SYSCALL_MEMMAP:
        th->flags &= ~SCHED_FLAG_KERNEL_ACCESS;
        res = (int)page_map((void*)param0, (void*)param1, (size_t)param2, (uint32_t)param3);
        break;

    case SYSCALL_MEMUNMAP:
        th->flags &= ~SCHED_FLAG_KERNEL_ACCESS;
        page_unmap((void*)param0, (size_t)param1);
        break;

    case SYSCALL_MEMALLOC:
        th->flags &= ~SCHED_FLAG_KERNEL_ACCESS;
        res = (int)page_alloc((size_t)param0, (uint32_t)param1);
        break;

    case SYSCALL_MEMFREE:
        th->flags &= ~SCHED_FLAG_KERNEL_ACCESS;
        page_free((void*)param0, (size_t)param1);
        break;

    default:
        res = -EINVAL;
        break;
    }

    th->flags &= ~SCHED_FLAG_KERNEL_ACCESS;
    return res;
}
