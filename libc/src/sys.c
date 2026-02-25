#include <sys.h>

int syscall(
    uint32_t id,
    uint32_t param0,
    uint32_t param1,
    uint32_t param2,
    uint32_t param3,
    uint32_t param4)
{
    int res = 0;
    __asm__ volatile("push %ebx");
    __asm__ volatile("push %ecx");
    __asm__ volatile("push %edx");
    __asm__ volatile("push %esi");
    __asm__ volatile("push %edi");
    __asm__ volatile("push %ebp");
    __asm__ volatile("mov %0, %%ebx" :: "r"(id));
    __asm__ volatile("mov %0, %%ecx" :: "r"(param0));
    __asm__ volatile("mov %0, %%edx" :: "r"(param1));
    __asm__ volatile("mov %0, %%esi" :: "r"(param2));
    __asm__ volatile("mov %0, %%edi" :: "r"(param3));
    __asm__ volatile("mov %0, %%ebp" :: "r"(param4));
    __asm__ volatile("int $0x80");
    __asm__ volatile("pop %ebp");
    __asm__ volatile("pop %edi");
    __asm__ volatile("pop %esi");
    __asm__ volatile("pop %edx");
    __asm__ volatile("pop %ecx");
    __asm__ volatile("mov %ebx, %eax");
    __asm__ volatile("pop %ebx");
    __asm__ volatile("mov %%eax, %0" : "=r"(res));
    return res;
}

int exit(int code)
{
    return syscall(1, (uint32_t)(code), 0, 0, 0, 0);
}

int usleep(size_t t)
{
    return syscall(2, (uint32_t)(t), 0, 0, 0, 0);
}

int open(handle_t *hndl, const char *path, int flags)
{
    return (int)syscall(4, (uint32_t)hndl, (uint32_t)path, (uint32_t)flags, 0, 0);
}

int close(handle_t hndl)
{
    return (int)syscall(5, (uint32_t)hndl, 0, 0, 0, 0);
}

int execp(pid_t *pid, const char *path)
{
    return syscall(6, (uint32_t)(pid), (uint32_t)(path), 0, 0, 0);
}

int read(handle_t hndl, void *buf, size_t len, size_t *rlen)
{
    return syscall(7, (uint32_t)(hndl), (uint32_t)(buf), (uint32_t)(len), (uint32_t)(rlen), 0);
}

int seek(handle_t hndl, size_t offset)
{
    return syscall(8, (uint32_t)(hndl), (uint32_t)(offset), 0, 0, 0);
}

int tell(handle_t hndl, size_t *offset)
{
    return syscall(9, (uint32_t)(hndl), (uint32_t)(offset), 0, 0, 0);
}

int size(handle_t hndl, size_t *size)
{
    return syscall(10, (uint32_t)(hndl), (uint32_t)(size), 0, 0, 0);
}

int get_pid(pid_t *pid)
{
    return (int)syscall(11, (uint32_t)pid, 0, 0, 0, 0);
}

int spawn_thread(tid_t *tid, pid_t pid, thread_entrypoint_t entrypoint)
{
    return (int)syscall(12, (uint32_t)tid, (uint32_t)pid, (uint32_t)entrypoint, 0, 0);
}

int get_tid(tid_t *tid)
{
    return (int)syscall(13, (uint32_t)tid, 0, 0, 0, 0);
}

int kill_thread(tid_t tid)
{
    return (int)syscall(14, (uint32_t)tid, 0, 0, 0, 0);
}

int mount(dev_t dev, const char *path)
{
    return (int)syscall(15, (uint32_t)dev, (uint32_t)path, 0, 0, 0);
}

int unmount(const char *path)
{
    return (int)syscall(16, (uint32_t)path, 0, 0, 0, 0);
}

int readdir(const char *path, struct fs_node *node)
{
    return (int)syscall(17, (uint32_t)path, (uint32_t)node, 0, 0, 0);
}

int write(handle_t hndl, const void *buf, size_t sz)
{
    return (int)syscall(18, (uint32_t)hndl, (uint32_t)buf, (uint32_t)sz, 0, 0);
}

int waitpid(pid_t pid)
{
    return (int)syscall(19, (uint32_t)pid, 0, 0, 0, 0);
}

int execpv(pid_t *pid, const char *path, int argc, char **argv)
{
    return syscall(20, (uint32_t)(pid), (uint32_t)(path), (uint32_t)(argc), (uint32_t)(argv), 0);
}

void *memalloc(size_t npages, uint32_t flags)
{
    return (int)syscall(23, (uint32_t)npages, (uint32_t)flags, 0, 0, 0);
}

void memfree(void *vbase, size_t npages)
{
    syscall(24, (uint32_t)vbase, (uint32_t)npages, 0, 0, 0);
}

void *memmap(void *vbase, void *pbase, size_t len, uint32_t flags)
{
    return (int)syscall(21, (uint32_t)vbase, (uint32_t)pbase, (uint32_t)len, (uint32_t)flags, 0);
}

void memunmap(void *vbase, size_t len)
{
    syscall(22, (uint32_t)vbase, (uint32_t)len, 0, 0, 0);
}

int ioctl(handle_t hndl, unsigned long req, void *arg)
{
    return (int)syscall(25, (uint32_t)hndl, (uint32_t)req, (uint32_t)arg, 0, 0);
}

int signal(pid_t pid, uint32_t signo)
{
    return (int)syscall(26, (uint32_t)pid, (uint32_t)signo, 0, 0, 0);
}

int signal_handler(proc_signal_t handler, uint32_t signo)
{
    return (int)syscall(27, (uint32_t)handler, (uint32_t)signo, 0, 0, 0);
}

void sigreturn()
{
    syscall(28, 0, 0, 0, 0, 0);
}

void reboot(void)
{
    syscall(29, 0, 0, 0, 0, 0);
}

void poweroff(void)
{
    syscall(30, 0, 0, 0, 0, 0);
}

int rmmod(const char *name)
{
    return (int)syscall(31, (uint32_t)name, 0, 0, 0, 0);
}

int insmod(const char *name)
{
    return (int)syscall(32, (uint32_t)name, 0, 0, 0, 0);
}

int lsmod(int idx, char *name, uintptr_t *ptr)
{
    return (int)syscall(33, (uint32_t)idx, (uint32_t)name, (uint32_t)ptr, 0, 0);
}

int lsproc(int idx, char *name, pid_t *pid, double *usg)
{
    return (int)syscall(34, (uint32_t)idx, (uint32_t)name, (uint32_t)pid, (uint32_t)usg, 0);
}