#include <kernel/kernel.h>

#include <kernel/sys/userio/keyboard.h>
#include <kernel/sys/sys.h>

void sys_init()
{
    printk("sys: initializing");

    sys_kb_init();
    // kernel_assert(devfs_init() == DEVFS_SUCCESS, "devfs init failed");
}
