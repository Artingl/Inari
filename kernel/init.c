#include <kernel/inari.h>
#include <misc/string.h>
#include <kernel/console/earlycon.h>

bootinfo_t bootinfo;

void kearly_init(bootinfo_t b)
{
    bootinfo = b;

    earlycon_init();


    printk("kearly_init: done");
}

void kmain(void)
{

}
