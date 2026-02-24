#include <kernel/inari.h>
#include <kernel/printk.h>
#include <kernel/errno.h>
#include <kernel/proc/sched.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/signals.h>
#include <kernel/sys/vfs.h>
#include <kernel/sys/syscall.h>
#include <kernel/console/console.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/pmm.h>

#include <arch/paging.h>

void arch_sched_sigreturn(struct thread *task);

int syscall_handle(
    uint32_t id,
    void *param0,
    void *param1,
    void *param2,
    void *param3,
    void *param4)
{
    size_t i;
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
    arch_switch_pagedir(proc->descriptor.vmem);

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
        /* Ensure the thread we try to kill is in our process */
        res = -EINVAL;
        for (i = 0; i < CONFIG_PROC_MAX_THREADS; i++)
            if (proc->threads[i] == (tid_t)param0)
            {
                res = sched_kill_thread((tid_t)param0);
                break;
            }

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
        if (th->state == SCHED_TASK_PAUSED)
        {
            sched_yield();
            
            if (th->flags & SCHED_FLAG_SYSCALL_RSLT)
            {
                res = th->syscall_result;
                th->flags &= ~SCHED_FLAG_SYSCALL_RSLT;
            }
        }
        break;

    case SYSCALL_EXECPV:
        res = execpv((pid_t*)param0, (const char*)param1, (int)param2, (char**)param3);
        break;

    case SYSCALL_MEMMAP:
        res = (int)arch_map_page(proc->descriptor.vmem, (void*)param0, (void*)param1, (size_t)param2, (uint32_t)param3);
        break;

    case SYSCALL_MEMUNMAP:
        arch_unmap_page(proc->descriptor.vmem, (void*)param0, (size_t)param1);
        break;

    case SYSCALL_MEMALLOC:
        res = (int)vmm_alloc_user(proc->descriptor.vmem, (size_t)param0);   // todo: ignores flags value (param1)
        break;

    case SYSCALL_MEMFREE:
        vmm_free_pages(proc->descriptor.vmem, (void*)param0, (size_t)param1);
        break;
    
    case SYSCALL_IOCTL:
        res = vfs_ioctl((vfs_handle_t)param0, (unsigned long)param1, (void*)param2);
        break;
    
    case SYSCALL_SIGNAL:
        /* Ignore hardware signals */
        if ((uint32_t)param1 == SIGSEGV || (uint32_t)param1 == SIGTRAP
            || (uint32_t)param1 == SIGTRAP || (uint32_t)param1 == SIGFPE
            || (uint32_t)param1 == SIGILL)
            { res = -EINVAL; break; }
        // /* Kill process without triggering signal */
        if ((uint32_t)param1 == SIGKILL)
            { res = kill_process((pid_t)param0, (uint32_t)-1); break; }

        res = proc_signal((pid_t)param0, (uint32_t)param1);
        break;
    
    case SYSCALL_INST_SIG:
        res = proc_install_signal(proc->pid, (proc_signal_t)param0, (uint32_t)param1);
        break;

    case SYSCALL_SIGRETURN:
        arch_sched_sigreturn(th);
        break;

    case 100: // debug
        printk("pmm: usage: %u MB (%u); total: %u MB (%u)", (pmm_usage() * 0x1000) / 1024 / 1024, pmm_usage(), (pmm_total() * 0x1000) / 1024 / 1024, pmm_total());
        break;

    default:
        proc_signal(proc->pid, SIGSYS);
        res = -EINVAL;
        break;
    }

    return res;
}
