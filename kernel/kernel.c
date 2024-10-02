#include <kernel/kernel.h>
#include <kernel/machine.h>
#include <kernel/tests/kernel_tests.h>
#include <kernel/core/module/module.h>
#include <kernel/driver/serial/serial.h>
#include <kernel/core/console/console.h>
#include <kernel/core/vfs/vfs.h>
#include <kernel/libc/math.h>
#include <kernel/libc/string.h>
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


static void test_vfs()
{
    // vfs_mount("/dev/")
}

#include <kernel/arch/i686/impl.h>

static void shenanigans()
{
    // 0xFFFFFFF0
    __inb(0x3DA);
    __outb(0x3C0, 0x10);
    __outb(0x3C0, 0x01);
    printk("switched mode");
    memset((void*)0xA0000, 0xff, 0xFFFF);

    while (1);
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

    vfs_init();
    kload_modules();
    // sys_init();

    test_vfs();

    // parse cmdline to initialize the kernel itself, and load all necessary kernel modules
    kparse_cmdline();

    shenanigans();

    panic("bsp: end reached");
    machine_halt();
}
