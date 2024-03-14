#include <kernel/kernel.h>
#include <kernel/lock/spinlock.h>
#include <kernel/sys/console/console.h>
#include <kernel/include/C/io.h>
#include <kernel/include/C/string.h>

#include <drivers/cpu/cpu.h>

#include <stdarg.h>

spinlock_t panic_spinlock = {0};

int do_printf_handler(char c, void **);
int printk_helper(const char *fmt, ...);

void panic(const char *message, ...)
{
    spinlock_acquire(&panic_spinlock);
    struct cpu_core *core = cpu_current_core();
    va_list args;
    va_start(args, message);
    printk_helper("[  %f] CPU#%d/PANIC :: ", kernel_uptime(), core->core_id);
    do_printkn(message, args, &do_printf_handler, NULL);

    // print the NL character so the screen would update
    console_printc('\n');
    va_end(args);

    machine_halt();
    spinlock_release(&panic_spinlock);
}

