#include <kernel/kernel.h>

#include <drivers/video/video.h>
#include <drivers/cpu/cpu.h>

#include <kernel/sys/console/console.h>

#include <kernel/include/C/io.h>
#include <kernel/include/C/string.h>

#include <stdarg.h>

int __pr_wrapper_handler(char c, void **)
{
    sys_console_printc(c);
}

// helper function for the "__pl_wrapper", to print formatted messages to the console
int __pr_wrapper_helper(const char *fmt, ...)
{
    int c;
    va_list args;
    va_start(args, fmt);
    c += do_printkn(fmt, args, &__pr_wrapper_handler, NULL);
    va_end(args);

    return c;
}

int __pr_wrapper(size_t line, const char *file, const char *func, const char *fmt, ...)
{
    int c = 0, shift = 0;
    char *prefix = NULL;

    // extract the prefix from the fmt
    switch (fmt[0])
    {
    case '0': { shift++; prefix = "CRIT"; break; }
    case '1': { shift++; prefix = "ERR"; break; }
    case '2': { shift++; prefix = "WARNING"; break; }
    case '3': { shift++; prefix = "NOTICE"; break; }
    case '4': { shift++; prefix = "INFO"; break; }
    case '5': { shift++; prefix = "DEBUG"; break; }
    case '6': { shift++; prefix = "TODO"; break; }
    default:  { prefix = "DEFAULT"; }
    }

    va_list args;
    va_start(args, fmt);
    // print formatted message to the console

    // prefix --------------
    if (fmt[0] == '6')
    { // different output for "TODO" prefix
        c += __pr_wrapper_helper("TODO{%f, %s:%d} :: ", kernel_uptime(), func, line);
    }
    else {
        c += __pr_wrapper_helper("[%f] %s :: ", kernel_uptime(), prefix);
    }
    // ----------------------

    // the message --------------
    c += do_printkn(fmt + shift, args, &__pr_wrapper_handler, NULL);
    c += sys_console_printc('\n');
    // --------------------------

    va_end(args);

    return c;
}

extern double __timer_ticks;

double kernel_uptime()
{
    return __timer_ticks / 1000.0f;// / __cpu_timer_freq();
}

void panic(const char *message, ...)
{
    va_list args;
    va_start(args, message);
    __pr_wrapper_helper("[%f] PANIC :: ", kernel_uptime());
    do_printkn(message, args, &__pr_wrapper_handler, NULL);

    // print the NL character so the screen would update
    sys_console_printc('\n');
    va_end(args);

    cpu_shutdown();
}
