#include <kernel/kernel.h>
#include <kernel/core/errno.h>
#include <kernel/core/syscall/syscall.h>

syscall_handler_t *handlers[KERN_MAX_SYSCALLS];

uint32_t kern_syscall_handle(uint8_t id, uint32_t param0, uint32_t param1, uint32_t param2, void *regs_ptr)
{
    if (id < KERN_MAX_SYSCALLS)
    {
        return handlers[id](id, param0, param1, param2, regs_ptr);
    }

    return 0;
}

void kern_syscall_register(uint8_t id, syscall_handler_t *handler)
{
    if (id >= KERN_MAX_SYSCALLS || handlers[id] != NULL)
    {
        printk("syscall: unable to register syscall with id %d", id);
        return;
    }

    handlers[id] = handler;

    return;
}
