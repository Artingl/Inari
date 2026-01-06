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

int debug(const char *s)
{
    return syscall(3, (uint32_t)(s), 0, 0, 0, 0);
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

int readdir(const char *path, struct fs_node *nodes, size_t offset, size_t limit)
{
    return (int)syscall(17, (uint32_t)path, (uint32_t)nodes, (uint32_t)offset, (uint32_t)limit, 0);
}