#include <arch/sys.h>
#include <kernel/sys/console.h>
#include <kernel/inari.h>
#include <kernel/proc/sched.h>
#include <kernel/subsys/video.h>
#include <kernel/sync/spinlock.h>

#include <arch/sys.h>

#include <misc/print.h>
#include <misc/string.h>

static spinlock_t panic_lock;
static int count = 0;

static inline void do_printf_handler(char c, void *) { console_write_system(&c, 1); }

static inline int print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int c = do_kprintfn(fmt, args, &do_printf_handler, NULL);
    va_end(args);
    return c;
}

static inline void helper_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    count += do_kprintfn(fmt, args, &do_printf_handler, NULL);
    va_end(args);
}

void panic(const char *fmt, ...) {
    sched_stop();
    console_switch_early();

    uint32_t flags;
    if (!spin_lock_is_free(&panic_lock)) {
        print("Nested panic!\n");
        halt();
    }

    spin_lock_irqsave(&panic_lock, flags);
#ifdef CONFIG_SUBSYS_VIDEO
    video_disable(); /* Disable video if possible */
#endif

    va_list args;
    va_start(args, fmt);
    count = 0;

    count += print("Panic:\n");
    count += do_kprintfn(fmt, args, &do_printf_handler, NULL);
    count += print("\n\n");
    print_stacktrace(&helper_printf);
    count += print("\n");
    count += print("Kernel panic!");

    va_end(args);

    spin_unlock_irqrestore(&panic_lock, flags);
    halt();
}
