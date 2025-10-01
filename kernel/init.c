#include <kernel/inari.h>
#include <kernel/console/earlycon.h>
#include <kernel/console/console.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>

#include <misc/string.h>

bootinfo_t bootinfo;

void kearly_init(bootinfo_t b)
{
    bootinfo = b;

    // Initialize the early console for atleast some output
    earlycon_init();

    pmm_init();
    vmm_init();

    uintptr_t addr = pmm_alloc_frames(1024);

    printk("allocated: 0x%08x", addr);
    
    printk("kearly_init: done");
}

void kmain(void)
{
    if (console_init() != 0)
        panic("Couldn't initialize console");

}

bootinfo_t get_boot_info()
{
    return bootinfo;
}
