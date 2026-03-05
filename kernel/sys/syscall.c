#include <kernel/console/console.h>
#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/kprintf.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/module.h>
#include <kernel/proc/ipc.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/sched.h>
#include <kernel/proc/signals.h>
#include <kernel/sys/syscall.h>
#include <kernel/sys/vfs.h>
#include <kernel/timer.h>
#include <kernel/uname.h>

#include <arch/paging.h>

void arch_sched_sigreturn(struct thread *task);
/* TODO: better pointer validation. Scenario: process calls `read`, passes userspace pointer to buffer, BUT the size
 * exceeds userspace and leaks onto kernelspace. */

int syscall_handle(uint32_t id, void *param0, void *param1, void *param2, void *param3, void *param4) {
    size_t i;
    tid_t tid;
    int res = 0;
    struct thread *th;
    if (sched_current_thread(&tid) != 0)
        return -ESRCH;
    if (sched_get_thread(tid, &th) != 0)
        return -ESRCH;
    if (!tid || !th->proc_data)
        return -ESRCH;
    struct process *proc = (struct process *)th->proc_data;
    arch_switch_pagedir(proc->descriptor.vmem);

    switch (id) {
    case SYSCALL_EXIT:
        res = exit(proc->pid, (int)param0);
        break;

    case SYSCALL_USLEEP:
        sched_usleep(tid, (size_t)param0);
        break;

    case SYSCALL_DEBUG:
        kprintf("pmm: usage: %u MB (%u); total: %u MB (%u)", (pmm_usage() * 0x1000) / 1024 / 1024, pmm_usage(),
                (pmm_total() * 0x1000) / 1024 / 1024, pmm_total());

        panic("test");
        break;

    case SYSCALL_OPEN:
        if (!VMM_IS_PTR_USERSPACE(param0) || !VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res = vfs_open((vfs_handle_t *)param0, (const char *)param1, (int)param2);
        break;

    case SYSCALL_CLOSE:
        res = vfs_close((vfs_handle_t)param0);
        break;

    case SYSCALL_EXECP:
        if (!VMM_IS_PTR_USERSPACE(param0) || !VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res = execp((pid_t *)param0, (const char *)param1);
        break;

    case SYSCALL_READ:
        if (!VMM_IS_PTR_USERSPACE(param1) || !VMM_IS_PTR_USERSPACE(param3)) {
            res = -EINVAL;
            break;
        }
        res = vfs_read((vfs_handle_t)param0, (void *)param1, (size_t)param2, (size_t *)param3);
        break;

    case SYSCALL_SEEK:
        res = vfs_seek((vfs_handle_t)param0, (size_t)param1);
        break;

    case SYSCALL_TELL:
        if (!VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res = vfs_tell((vfs_handle_t)param0, (size_t *)param1);
        break;

    case SYSCALL_SIZE:
        if (!VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res = vfs_size((vfs_handle_t)param0, (size_t *)param1);
        break;

    case SYSCALL_GET_PID:
        if (!VMM_IS_PTR_USERSPACE(param0)) {
            res = -EINVAL;
            break;
        }
        if (param0)
            *((pid_t *)param0) = proc->pid;
        break;

    case SYSCALL_SPAWN_THREAD:
        if (!VMM_IS_PTR_USERSPACE(param0) || !VMM_IS_PTR_USERSPACE(param2)) {
            res = -EINVAL;
            break;
        }
        /* Can only spawn threads for itself */
        if ((pid_t)param1 != proc->pid) {
            res = -EINVAL;
            break;
        }
        return spawn_thread((tid_t *)param0, (pid_t)param1, (thread_entrypoint_t)param2);

    case SYSCALL_GET_TID:
        if (!VMM_IS_PTR_USERSPACE(param0)) {
            res = -EINVAL;
            break;
        }
        if (param0)
            *((pid_t *)param0) = th->tid;
        break;

    case SYSCALL_KILL_THREAD:
        /* Ensure the thread we try to kill is in our process */
        res = -EINVAL;
        for (i = 0; i < CONFIG_PROC_MAX_THREADS; i++)
            if (proc->threads[i] == (tid_t)param0) {
                res = sched_kill_thread((tid_t)param0);
                break;
            }

        break;

    case SYSCALL_MOUNT:
        if (!VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res = vfs_mount((dev_t)param0, (const char *)param1);
        break;

    case SYSCALL_UNMOUNT:
        if (!VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res = vfs_unmount((const char *)param1);
        break;

    case SYSCALL_READDIR:
        if (!VMM_IS_PTR_USERSPACE(param0) || !VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res = vfs_readdir((const char *)param0, (struct vfs_node *)param1);
        break;

    case SYSCALL_WRITE:
        if (!VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res = vfs_write((vfs_handle_t)param0, (void *)param1, (size_t)param2);
        break;

    case SYSCALL_WAITPID:
        res = waitpid((pid_t)param0, th->tid);
        if (th->state == SCHED_TASK_PAUSED) {
            sched_yield();

            if (th->flags & SCHED_FLAG_SYSCALL_RSLT) {
                res = th->syscall_result;
                th->flags &= ~SCHED_FLAG_SYSCALL_RSLT;
            }
        }
        break;

    case SYSCALL_EXECPV:
        if (!VMM_IS_PTR_USERSPACE(param0) || !VMM_IS_PTR_USERSPACE(param1) || !VMM_IS_PTR_USERSPACE(param3)) {
            res = -EINVAL;
            break;
        }
        res = execpv((pid_t *)param0, (const char *)param1, (int)param2, (char **)param3);
        break;

    case SYSCALL_MEMMAP:
        if (!VMM_IS_PTR_USERSPACE(param0) || !VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res =
            (int)arch_map_page(proc->descriptor.vmem, (void *)param0, (void *)param1, (size_t)param2, (uint32_t)param3);
        break;

    case SYSCALL_MEMUNMAP:
        if (!VMM_IS_PTR_USERSPACE(param0)) {
            res = -EINVAL;
            break;
        }
        arch_unmap_page(proc->descriptor.vmem, (void *)param0, (size_t)param1);
        break;

    case SYSCALL_MEMALLOC:
        res = (int)vmm_alloc_user(proc->descriptor.vmem, (size_t)param0); // todo: ignores flags value (param1)
        break;

    case SYSCALL_MEMFREE:
        if (!VMM_IS_PTR_USERSPACE(param0)) {
            res = -EINVAL;
            break;
        }
        vmm_free_pages(proc->descriptor.vmem, (void *)param0, (size_t)param1);
        break;

    case SYSCALL_IOCTL:
        /* TODO: each ioctl handler must check the param2 */
        res = vfs_ioctl((vfs_handle_t)param0, (unsigned long)param1, (void *)param2);
        break;

    case SYSCALL_SIGNAL:
        /* Ignore hardware signals */
        if ((uint32_t)param1 == SIGSEGV || (uint32_t)param1 == SIGTRAP || (uint32_t)param1 == SIGTRAP ||
            (uint32_t)param1 == SIGFPE || (uint32_t)param1 == SIGILL) {
            res = -EINVAL;
            break;
        }
        // /* Kill process without triggering signal */
        if ((uint32_t)param1 == SIGKILL) {
            res = kill_process((pid_t)param0, (uint32_t)-1);
            break;
        }

        res = proc_signal((pid_t)param0, (uint32_t)param1);
        break;

    case SYSCALL_INST_SIG:
        res = proc_install_signal(proc->pid, (proc_signal_t)param0, (uint32_t)param1);
        break;

    case SYSCALL_SIGRETURN:
        arch_sched_sigreturn(th);
        break;

    case SYSCALL_REBOOT:
        kpower_off(1);
        break;

    case SYSCALL_POWEROFF:
        kpower_off(0);
        break;

    case SYSCALL_RMMOD:
        if (!VMM_IS_PTR_USERSPACE(param0)) {
            res = -EINVAL;
            break;
        }
        res = modules_rmmod((const char *)param0);
        break;

    case SYSCALL_INSMOD:
        if (!VMM_IS_PTR_USERSPACE(param0)) {
            res = -EINVAL;
            break;
        }
        res = modules_insmod((const char *)param0);
        break;

    case SYSCALL_LSMOD:
        if (!VMM_IS_PTR_USERSPACE(param1) || !VMM_IS_PTR_USERSPACE(param2) || !VMM_IS_PTR_USERSPACE(param3)) {
            res = -EINVAL;
            break;
        }
        res = modules_ls((int)param0, (char *)param1, (uintptr_t *)param2, (uint32_t *)param3);
        break;

    case SYSCALL_LSPROC:
        if (!VMM_IS_PTR_USERSPACE(param1) || !VMM_IS_PTR_USERSPACE(param2) || !VMM_IS_PTR_USERSPACE(param3)) {
            res = -EINVAL;
            break;
        }
        res = proc_ls((int)param0, (char *)param1, (pid_t *)param2, (double *)param3);
        break;

    case SYSCALL_FLUSH:
        res = vfs_flush((vfs_handle_t)param0);
        break;

    case SYSCALL_UNAME:
        if (!VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res = uname((struct utsname *)param0);
        break;

    case SYSCALL_TIME:
        if (!VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        *((time_t *)param1) = (timer_get_ticks() * 1000) / timer_get_resolution() * 1000;
        res = 0;
        break;

    case SYSCALL_IPC_CREATE:
        if (!VMM_IS_PTR_USERSPACE(param0) || !VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res = ipc_create(proc, (const char *)param0, (thread_entrypoint_t)param1);
        break;
    case SYSCALL_IPC_FREE:
        if (!VMM_IS_PTR_USERSPACE(param0)) {
            res = -EINVAL;
            break;
        }
        res = ipc_free(proc, (const char *)param0);
        break;
    case SYSCALL_IPC_FETCH_NEXT:
        if (!VMM_IS_PTR_USERSPACE(param0) || !VMM_IS_PTR_USERSPACE(param1) || !VMM_IS_PTR_USERSPACE(param2) ||
            !VMM_IS_PTR_USERSPACE(param3) || !VMM_IS_PTR_USERSPACE(param4)) {
            res = -EINVAL;
            break;
        }
        res = ipc_fetch_next(proc, th, (pid_t *)param0, (ipc_handle_t *)param1, (uint32_t *)param2, (void **)param3,
                             (size_t *)param4);
        break;
    case SYSCALL_IPC_REPLY:
        res = ipc_reply(proc, th, (int)param0);
        break;
    case SYSCALL_IPC_OPEN:
        if (!VMM_IS_PTR_USERSPACE(param0) || !VMM_IS_PTR_USERSPACE(param1)) {
            res = -EINVAL;
            break;
        }
        res = ipc_open(proc, (const char *)param0, (ipc_handle_t *)param1);
        break;
    case SYSCALL_IPC_CLOSE:
        res = ipc_close(proc, (ipc_handle_t)param0);
        break;
    case SYSCALL_IPC_SEND:
        if (!VMM_IS_PTR_USERSPACE(param2)) {
            res = -EINVAL;
            break;
        }
        res = ipc_send(proc, (ipc_handle_t)param0, (uint32_t)param1, (void *)param2, (size_t)param3);
        break;
    case SYSCALL_IPC_WAIT:
        res = ipc_wait(proc, (ipc_handle_t)param0, (uint8_t)param1);
        break;

    default:
        proc_signal(proc->pid, SIGSYS);
        res = -EINVAL;
        break;
    }

    sched_yield();

    return res;
}
