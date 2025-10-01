#include <kernel/inari.h>
#include <kernel/console/earlycon.h>
#include <kernel/console/console.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>

#include <arch/paging.h>
#include <misc/string.h>

bootinfo_t bootinfo;

void kearly_init(bootinfo_t b)
{
    bootinfo = b;

    // Initialize the early console for atleast some output
    earlycon_init();

    pmm_init();
    vmm_init();

    printk("kearly_init: done 0x%02x; 0x%08x 0x%08x");
}

void kmain(void)
{
    printk("Inari kernel cmdline: %s", bootinfo.cmdline);
    printk("Available memory %ldkb", (pmm_total() - pmm_usage()) * (KERN_PAGE_SIZE / 1024));

    if (console_init() != 0)
        panic("Couldn't initialize console");


    panic("Reached end of kmain");
}

bootinfo_t get_boot_info()
{
    return bootinfo;
}
