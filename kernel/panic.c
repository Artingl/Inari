#include <kernel/kernel.h>
#include <kernel/machine.h>
#include <kernel/lock/spinlock.h>
#include <kernel/sys/console/console.h>
#include <kernel/include/C/io.h>
#include <kernel/include/C/string.h>

#include <stdarg.h>

spinlock_t panic_spinlock = {0};

void __console_printc(char c);

int do_panic_handler(char c, void **_)
{
    __console_printc(c);
}

int panic_helper(const char *fmt, ...)
{
    int c;
    va_list args;
    va_start(args, fmt);
    c += do_printkn(fmt, args, &do_panic_handler, NULL);
    va_end(args);

    return c;
}

void panic(const char *message, ...)
{
    spinlock_acquire(&panic_spinlock);
    va_list args;
    va_start(args, message);
    panic_helper("[  %f] PANIC: ", kernel_time() / 1000.0f);
    do_printkn(message, args, &do_panic_handler, NULL);

    __console_printc('\n');
    va_end(args);

    spinlock_release(&panic_spinlock);
    machine_halt();
}

