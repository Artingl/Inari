#include <kernel/inari.h>
#include <arch/sys.h>
#include <kernel/console/console.h>
#include <kernel/sched/sched.h>

#include <misc/print.h>
#include <misc/string.h>

static inline void do_printf_handler(char c)
{
    console_printc(CONSOLE_PANIC, &c, 1);
}

static inline int print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int c = do_printkn(fmt, args, &do_printf_handler);
    va_end(args);
    return c;
}

void panic(const char *fmt, ...)
{
    sched_stop();

    va_list args;
    va_start(args, fmt);
    int c = 0;
    
    c += print("Panic:\n");
    c += do_printkn(fmt, args, &do_printf_handler);
    c += print("\n");

    c += print("\n");
    c += print("Kernel panic!");
    
    va_end(args);
    while (1){}
    // halt();
}
