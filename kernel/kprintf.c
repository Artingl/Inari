#include <kernel/inari.h>
#include <kernel/timer.h>
#include <kernel/console/console.h>

#include <misc/print.h>
#include <misc/string.h>

#include <arch/sys.h>

static inline void do_printf_handler(char c, void*)
{
    console_puts(CONSOLE_PRINT, &c, 1);
}

static inline int print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int c = do_kprintfn(fmt, args, &do_printf_handler, NULL);
    va_end(args);
    return c;
}

int kprintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int c = 0;

    if (is_cpu_initialized())   /* TODO: Doing division crashes the kernel on bare metal i686 */
        c += print("[  %f ] ", (double)timer_get_ticks() / (double)timer_get_resolution());
    else
        c += print("[  0.000000 ] ");
    c += do_kprintfn(fmt, args, &do_printf_handler, NULL);
    c += print("\n");

    console_flush();
    va_end(args);
    return c;
}

