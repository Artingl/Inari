#include <kernel/kernel.h>
#include <kernel/lock/spinlock.h>
#include <kernel/sys/console/console.h>

#include <drivers/cpu/cpu.h>

#include <stdarg.h>

static volatile spinlock_t prink_spinlock = {0};

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
    spinlock_acquire(&prink_spinlock);
    int c = 0, shift = 0;
    char *prefix = NULL;
    struct cpu_core *core = cpu_current_core();

    // extract the prefix from the fmt
    switch (fmt[0])
    {
    case '0':
    {
        shift++;
        prefix = "CRIT";
        break;
    }
    case '1':
    {
        shift++;
        prefix = "ERR";
        break;
    }
    case '2':
    {
        shift++;
        prefix = "WARNING";
        break;
    }
    case '3':
    {
        shift++;
        prefix = "NOTICE";
        break;
    }
    case '4':
    {
        shift++;
        prefix = "INFO";
        break;
    }
    case '5':
    {
#ifdef CONFIG_NODEBUG
        return 0;
#endif
        shift++;
        prefix = "DEBUG";
        break;
    }
    case '6':
    {
        shift++;
        prefix = "TODO";
        break;
    }
    default:
    {
        prefix = "DEFAULT";
    }
    }

    va_list args;
    va_start(args, fmt);

    c += printk_helper("[  %f] CPU#%d/%s :: ", kernel_uptime(), core->core_id, prefix);
    c += do_printkn(fmt + shift, args, &do_printf_handler, NULL);
    c += console_printc('\n');

    va_end(args);
    spinlock_release(&prink_spinlock);
    return c;
}
