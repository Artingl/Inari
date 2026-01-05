#include <kernel/inari.h>
#include <kernel/printk.h>
#include <kernel/errno.h>
#include <kernel/sys/syscall.h>


int syscall_handle(
    uint32_t id,
    uint32_t param0,
    uint32_t param1,
    uint32_t param2,
    uint32_t param3,
    uint32_t param4)
{
    printk("syscall: id %u; params 0x%x 0x%x 0x%x 0x%x 0x%x",
            id, param0, param1, param2, param3, param4);
    
    switch (id)
    {
    case SYSCALL_EXIT:
        printk("exit");
        break;
    
    case 23:
        printk(param0);
        break;

    default:
        return -EINVAL;
    }

    return 0;
}
