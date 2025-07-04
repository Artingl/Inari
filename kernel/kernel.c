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

#include <kernel/core/sched/scheduler.h>

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

    scheduler_init();

    vfs_init();
    kload_modules();
    // sys_init();

    test_vfs();

    // parse cmdline to initialize the kernel itself, and load all necessary kernel modules
    kparse_cmdline();

    void entrycode_a1();
    void entrycode_b1();
    void entrycode_c1();
    
    scheduler_create_task(&entrycode_a1);
    scheduler_create_task(&entrycode_b1);
    // scheduler_create_task(&entrycode_c1);

    kernel_assert(scheduler_enter() != 0, "Unable to run the scheduler on BSP!");

    panic("bsp: end reached");
    machine_halt();
}

void serial_putc(uint16_t port, uint8_t c);

void entrycode_a1()
{
    volatile uint32_t i = 0;
    while (1) {
        printk("task 2 printing");
        // serial_putc(1, 'a');
        *((uint8_t*)0xb8000) = i++;
        *((uint8_t*)0xb8001) = (i + 60);
    }
}

void entrycode_b1()
{
    volatile uint32_t i = 128;
    while (1) {
        printk("task 1 printing");
        // serial_putc(1, 'b');
        *((uint8_t*)0xb8004) = i++;
        *((uint8_t*)0xb8005) = (i + 60);
    }
}

void entrycode_c1()
{
    while (1) {
        serial_putc(1, 'c');
    }
}
