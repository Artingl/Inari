#include <kernel/inari.h>
#include <kernel/console/earlycon.h>
#include <kernel/console/console.h>
#include <kernel/time.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/page.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/sched/sched.h>
#include <kernel/module.h>

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
    printk("Inari kernel cmdline: %s", bootinfo.cmdline);

    assert(console_init() == 0, "console init failed.");
    assert(sched_init() == 0, "scheduler init failed.");

    void test();
    test();
    
    modules_init();
    enable_int();
    sched_enter_core();
    panic("Reached end of kmain");
}

void test_task1()
{
    int i = 0;
    while (1)
    {
        usleep(1000000);
        printk("task 1 %d", i++);
    }
}

void test_task2()
{
    int i = 0;
    while (1)
    {
        usleep(1000000);
        printk("task 2 %d", i++);
    }
}

void test()
{
    tid_t task_id;
    sched_add_task(&task_id, &test_task1);
    sched_add_task(&task_id, &test_task2);
    printk("test: task %u", task_id);
}

bootinfo_t get_boot_info()
{
    return bootinfo;
}
