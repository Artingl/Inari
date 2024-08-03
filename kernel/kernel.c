#include <kernel/kernel.h>
#include <kernel/machine.h>
#include <kernel/tests/kernel_tests.h>
#include <kernel/module/module.h>
#include <kernel/driver/serial/serial.h>
#include <kernel/sys/console/console.h>
#include <kernel/include/C/math.h>
#include <kernel/include/C/string.h>
#include <kernel/driver/interrupt/interrupt.h>

static char *_cmdline;

extern char __kvirtual_start;
extern char __kvirtual_end;

const char *kernel_cmdline()
{
    return _cmdline;
}

extern double cpu_timer_ticks;
double kernel_time()
{
    return cpu_timer_ticks; // / cpu_timer_freq();
}

void kmain(char *cmdline)
{
    _cmdline = (char*)cmdline;

    kern_interrupts_init();
    console_late_init();

    // Make some small kernel tests
    // do_kern_tests();

    // video_init();

    console_clear();
    printk("Inari kernel: cmdline: %s", kernel_cmdline());

    kload_modules();
    // sys_init();

    // initialize drivers
    // ps2_init();

    // parse cmdline to initialize the kernel itself, and load all necessary kernel modules
    kparse_cmdline();


    panic("bsp: end reached");
    machine_halt();
}
