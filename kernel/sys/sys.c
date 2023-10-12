#include <kernel/kernel.h>

#include <kernel/sys/userio/keyboard.h>
#include <kernel/sys/sys.h>

void sys_drv_init()
{
    printk(KERN_INFO "Initializing sys modules.");

    sys_drv_kb_init();
}
