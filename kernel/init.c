#include <kernel/inari.h>
#include <kernel/console/earlycon.h>
#include <kernel/console/console.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/page.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/sched/sched.h>
#include <kernel/module.h>

#include <arch/paging.h>
#include <misc/string.h>

bootinfo_t bootinfo;

#define assert(cond, fmt, ...) {if (!(cond)) {panic(fmt, ##__VA_ARGS__);}}

void kearly_init(bootinfo_t b)
{
    bootinfo = b;

    /* Initialize the early console for atleast some output */
    earlycon_init();

    if (pmm_init() || vmm_init() || page_init() || kmalloc_init())
        panic("Unable to initialize the memory subsystem.");

    printk("kearly_init: done");
}

void kmain(void)
{
    printk("Inari kernel cmdline: %s", bootinfo.cmdline);

    assert(console_init() == 0, "Couldn't initialize console");
    assert(sched_init() == 0, "Couldn't initialize scheduler");

    void test();
    test();
    
    modules_init();
    sched_enter_core();
    panic("Reached end of kmain");
}

void usdelay(size_t us)
{
    // TODO: stub
    for (volatile size_t i = 0; i < 0xffffff; i++)
    {}
}

void test_task1()
{
    int i = 0;
    // *((uint8_t*)0x47347473) = 0;
    // while (1) {
    //     printk("test task 1 %d", i++);
    // }
}


void test_task2()
{
    int i = 0;
    while (1) {
        printk("test task 2 %d", i++);
    }
}

void test()
{
    tid_t task_id;
    sched_add_task(&task_id, &test_task1);
    // sched_add_task(&task_id, &test_task2);
    printk("test: task %u", task_id);
}

bootinfo_t get_boot_info()
{
    return bootinfo;
}
