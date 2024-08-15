#include <kernel/kernel.h>
#include <kernel/machine.h>
#include <kernel/tests/kernel_tests.h>
#include <kernel/core/module/module.h>
#include <kernel/driver/serial/serial.h>
#include <kernel/core/console/console.h>
#include <kernel/include/math.h>
#include <kernel/include/string.h>
#include <kernel/driver/interrupt/interrupt.h>

static char *_cmdline;

extern char __kvirtual_start;
extern char __kvirtual_end;

const char *kernel_cmdline()
{
    return _cmdline;
}

double kernel_uptime_ticks;
double kernel_time()
{
    return kernel_uptime_ticks; // / cpu_timer_freq();
}

void kmain(char *cmdline)
{
    _cmdline = (char*)cmdline;

    kern_interrupts_init();
    console_late_init();

    // Make some small kernel tests
    do_kern_tests();

    // video_init();

    console_clear();
    printk("Inari kernel: cmdline: %s", kernel_cmdline());

    kload_modules();
    // sys_init();

    // parse cmdline to initialize the kernel itself, and load all necessary kernel modules
    kparse_cmdline();

    panic("bsp: end reached");
    machine_halt();
}
