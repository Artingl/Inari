#include <kernel/inari.h>
#include <kernel/console/earlycon.h>
#include <kernel/console/console.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/page.h>
#include <kernel/mm/kmalloc.h>

#include <arch/paging.h>
#include <misc/string.h>

bootinfo_t bootinfo;

void kearly_init(bootinfo_t b)
{
    bootinfo = b;

    // Initialize the early console for atleast some output
    earlycon_init();

    if (pmm_init() || vmm_init() || page_init() || kmalloc_init())
        panic("Unable to initialize the memory subsystem.");

    printk("kearly_init: done");
}

void kmain(void)
{
    printk("Inari kernel cmdline: %s", bootinfo.cmdline);

    if (console_init() != 0)
        panic("Couldn't initialize console");
    
    panic("Reached end of kmain");
}

bootinfo_t get_boot_info()
{
    return bootinfo;
}
