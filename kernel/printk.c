#include <kernel/kernel.h>
#include <kernel/lock/spinlock.h>
#include <kernel/sys/console/console.h>
#include <kernel/include/C/io.h>

#include <stdarg.h>


int do_printf_handler(char c, void **)
{
    console_printc(c);
}

int printk_helper(const char *fmt, ...)
{
    int c;
    va_list args;
    va_start(args, fmt);
    c += do_printkn(fmt, args, &do_printf_handler, NULL);
    va_end(args);

    return c;
}

int printk_wrapper(size_t line, const char *file, const char *func, const char *fmt, ...)
{
    int c = 0, shift = 0;
    char *prefix = NULL;

    // extract the prefix from the fmt
    switch (fmt[0])
    {
    case '0':
    {
        shift++;
        prefix = " ERROR:";
        break;
    }
    case '1':
    {
        shift++;
        prefix = " WARNING:";
        break;
    }
    default:
    {
        prefix = "";
    }
    }

    va_list args;
    va_start(args, fmt);

    c += printk_helper("[  %f]%s ", kernel_uptime(), prefix);
    c += do_printkn(fmt + shift, args, &do_printf_handler, NULL);
    c += console_printc('\n');

    va_end(args);
    return c;
}
