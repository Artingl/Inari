#include <kernel/inari.h>
#include <kernel/proc/sched.h>
#include <kernel/timer.h>

static volatile size_t timer_resolution = 1;
static volatile size_t timer_ticks;

int timer_init(size_t resolution) {
    timer_resolution = resolution;
    timer_ticks = 0;

    return 0;
}

void timer_tick() { timer_ticks++; }

size_t timer_get_ticks() { return timer_ticks; }

size_t timer_get_resolution() { return timer_resolution; }

size_t uptimer_ms() { return (timer_ticks * 1000) / timer_resolution; }

void usleep(size_t us) {
    tid_t tid;
    if (sched_current_thread(&tid) == 0) {
        /* Called from a task, use scheduler to sleep */
        sched_usleep(tid, us);
        sched_yield();
        return;
    }

    size_t ticks_start = (timer_ticks * 1000) / timer_resolution * 1000 + us;
    while (ticks_start > (timer_ticks * 1000) / timer_resolution * 1000)
        ;
}