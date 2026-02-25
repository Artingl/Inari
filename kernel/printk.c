#include <kernel/inari.h>
#include <kernel/timer.h>
#include <kernel/console/console.h>

#include <misc/print.h>
#include <misc/string.h>

#include <arch/sys.h>

static inline void do_printf_handler(char c, void*)
{
    console_printc(CONSOLE_PRINTK, &c, 1);
}

static inline int print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int c = do_printkn(fmt, args, &do_printf_handler, NULL);
    va_end(args);
    return c;
}

int printk(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int c = 0;

    // if (is_cpu_initialized() && int_is_enabled())
    //     c += print("[  %f ] ", (double)timer_get_ticks() / (double)timer_get_resolution());
    // else
        c += print("[  0.000000 ] ");
    c += do_printkn(fmt, args, &do_printf_handler, NULL);
    c += print("\n");

    va_end(args);
    return c;
}

