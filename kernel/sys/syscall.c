#include <kernel/inari.h>
#include <kernel/printk.h>
#include <kernel/errno.h>
#include <kernel/proc/sched.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/signals.h>
#include <kernel/sys/vfs.h>
#include <kernel/sys/syscall.h>
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
    struct thread *th;
    if (sched_current_thread(&tid) != 0)
        return -ESRCH;
    if (sched_get_thread(tid, &th) != 0)
        return -ESRCH;
    if (!tid || !th->proc_data)
        return -ESRCH;
    struct process *proc = (struct process*)th->proc_data;

    // printk("syscall: id %u; params 0x%x 0x%x 0x%x 0x%x 0x%x",
    //         id, param0, param1, param2, param3, param4);
    
    switch (id)
    {
    case SYSCALL_EXIT:
        return exit(proc->pid, (int)param0);
    
    case SYSCALL_USLEEP:
        sched_usleep(tid, (size_t)param0);
        break;

    // case SYSCALL_DEBUG:
    //     return printk(param0);
    
    case SYSCALL_OPEN:
        return vfs_open((vfs_handle_t*)param0, (const char*)param1, (int)param2);

    case SYSCALL_CLOSE:
        return vfs_close((vfs_handle_t)param0);

    case SYSCALL_EXECP:
        return execp((pid_t*)param0, (const char*)param1);
    
    case SYSCALL_READ:
        return vfs_read((vfs_handle_t)param0, (void*)param1, (size_t)param2, (size_t*)param3);
        
    case SYSCALL_SEEK:
        return vfs_seek((vfs_handle_t)param0, (size_t)param1);
        
    case SYSCALL_TELL:
        return vfs_tell((vfs_handle_t)param0, (size_t*)param1);
        
    case SYSCALL_SIZE:
        return vfs_size((vfs_handle_t)param0, (size_t*)param1);
    
    case SYSCALL_GET_PID:
        if (param0)
            *((pid_t*)param0) = proc->pid;
        return 0;

    case SYSCALL_SPAWN_THREAD:
        return spawn_thread((tid_t*)param0, (pid_t)param1, (thread_entrypoint_t)param2);
    
    case SYSCALL_GET_TID:
        if (param0)
            *((pid_t*)param0) = th->tid;
        return 0;

    case SYSCALL_KILL_THREAD:
        /* NOTE: SIGKILL will kill a thread immediately */
        return sched_signal_thread((tid_t)param0, SIGKILL);

    case SYSCALL_MOUNT:
        return vfs_mount((dev_t)param0, (const char*)param1);

    case SYSCALL_UNMOUNT:
        return vfs_unmount((const char*)param1);
    
    case SYSCALL_READDIR:
        return vfs_readdir((const char*)param0, (struct vfs_node*)param1);
    
    case SYSCALL_WRITE:
        return vfs_write((vfs_handle_t)param0, (void*)param1, (size_t)param2);

    default:
        return -EINVAL;
    }

    return 0;
}
