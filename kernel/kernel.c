#include <kernel/kernel.h>
#include <kernel/tests/kernel_tests.h>
#include <kernel/module/module.h>
#include <drivers/serial/serial.h>
#include <kernel/sys/console/console.h>
#include <kernel/include/C/math.h>
#include <kernel/include/C/string.h>

#include <kernel/arch/i386/impl.h>

// the payload should be in the lower memory so we can use it anywhere we want even without paging
__attribute__((section(".lo_text"))) struct kernel_payload payload;


extern char __kvirtual_start;
extern char __kvirtual_end;

struct kernel_payload const *kernel_configuration()
{
    return &payload;
}

extern double cpu_timer_ticks;
double kernel_time()
{
    return cpu_timer_ticks; // / cpu_timer_freq();
}

void kmain()
{
    console_init(SERIAL_COM0);

    panic("end here");

    // Make some small kernel tests
    do_kern_tests();

    console_enable_heap();
    // video_init();

    console_clear();
    printk("Inari kernel (x86, %s)", payload.bootloader);
    printk("Kernel cmdline: %s", payload.cmdline);
    printk("Kernel virtual start: %p", &__kvirtual_start);
    printk("Kernel virtual end: %p", &__kvirtual_end);

    kload_modules();
    // sys_init();

    // initialize drivers
    // ps2_init();

    // parse cmdline to initialize the kernel itself, and load all necessary kernel modules
    kparse_cmdline();


    panic("bsp: end reached");
    __halt();
}
