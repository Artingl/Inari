#include <kernel/inari.h>
#include <kernel/console/earlycon.h>
#include <kernel/console/console.h>
#include <kernel/timer.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/page.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/sys/block.h>
#include <kernel/sys/driver.h>
#include <kernel/sched/sched.h>
#include <kernel/module.h>
#include <kernel/vfs/vfs.h>
#include <kernel/event.h>

#include <arch/sys.h>
#include <arch/paging.h>
#include <misc/string.h>

bootinfo_t bootinfo;

#define assert(cond, fmt, ...) {if (!(cond)) {panic(fmt, ##__VA_ARGS__);}}

void kearly_init(bootinfo_t b)
{
    bootinfo = b;

    /* Initialize the early console for atleast some output */
    earlycon_init();

    assert(pmm_init() == 0, "pmm init failed.");
    assert(vmm_init() == 0, "vmm init failed.");
    assert(page_init() == 0, "paging init failed.");
    assert(kmalloc_init() == 0, "kmalloc/kfree init failed.");

    printk("kearly_init: done");
}

void kmain(void)
{
    assert(console_init() == 0, "console init failed.");
    assert(event_bus_init() == 0, "event bus init failed.");
    assert(blkdev_init() == 0, "block device init failed.");
    assert(sched_init() == 0, "scheduler init failed.");
    assert(vfs_init() == 0, "vfs init failed.");

    printk("Inari kernel cmdline: %s", bootinfo.cmdline);

    void test();
    test();

    enable_int();
    modules_init();
    sched_enter_core();
    panic("Reached end of kmain");
}

void test_task()
{
    struct block_device *device = block_get(MKDEV(GPT_DRIVER, 1));
    if (!device)
    {
        printk("test: partition not found");
        return;
    }

    uint8_t buffer[512];
    device->ops->read_blocks(device, 0, (void*)&buffer[0], 1);

    printk("test: 0x%08x 0x%08x 0x%08x", buffer[0], buffer[1], buffer[2]);
}

void cpu_usage_task()
{
    while (1)
    {
        usleep(2000000);
        for (size_t i = 0; i < 10000; i++)
        {
            struct sched_task *task;
            if (sched_get_task(i, &task) == 0)
            {
                if (task->reschedules_count > 0 && task->cpu_time > 0)
                    printk("cpu_usage: tid %lu time %lu count %lu", i,
                        task->cpu_time, task->reschedules_count);
            }
        }
    }
}

void test()
{
    tid_t task_id;
    sched_add_task(&task_id, &test_task);
    sched_add_task(&task_id, &cpu_usage_task);
}

bootinfo_t get_boot_info()
{
    return bootinfo;
}
