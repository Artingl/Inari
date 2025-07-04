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

uint64_t kernel_uptime_ticks = 0;
uint64_t kernel_time()
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

    // Make some small kernel tests
    do_kern_tests();

    // video_init();

    console_clear();
    printk("Inari kernel: cmdline: %s", kernel_cmdline());

    scheduler_init();

    vfs_init();
    test_vfs();

    kern_modules_init();

    void test11(void*);
    scheduler_create_task(&test11, NULL);

    // parse cmdline to initialize the kernel itself, and load all necessary kernel modules
    // kern_parse_cmdline();

    kernel_assert(scheduler_enter() != 0, "Unable to run the scheduler on BSP!");
    panic("kennel: bsp end reached");
    machine_halt();
}

#include <kernel/core/syscall/syscall.h>
void test11(void*fd)
{
    printk("started");
    kern_syscall(KERN_SYSCALL_SLEEP, 1000 * 2000, 0, 0);
    kern_module_unload("vconsole");
    printk("ended");
}

void kern_shutdown()
{
    printk("kernel: shutting down");

    kern_modules_cleanup();
}
