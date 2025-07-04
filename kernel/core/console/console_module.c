#include <kernel/kernel.h>
#include <kernel/core/syscall/syscall.h>
#include <kernel/core/console/console.h>
#include <kernel/core/module/module.h>
#include <kernel/core/errno.h>

#include <kernel/machine.h>

int console_module_entrypoint();
int console_module_event(uint32_t event_id, void *payload);

void console_load_module()
{
    kern_module_load(
        "vconsole",
        &console_module_entrypoint,
        &console_module_event
    );
}

int console_module_entrypoint()
{
    printk("CONSOLE!!!");
    kern_syscall(KERN_SYSCALL_SLEEP, 1000 * 500, 0, 0);
    printk("done!!!");
    // machine_poweroff();

    return K_OKAY;
}

int console_module_event(uint32_t event_id, void *payload)
{
    struct sched_task *task = scheduler_get_current_task();
    printk("console got event %d 0x%x %d", event_id, payload, task->tid);
}
