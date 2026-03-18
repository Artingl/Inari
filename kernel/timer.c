#include <kernel/inari.h>
#include <kernel/proc/sched.h>
#include <kernel/timer.h>

static volatile uint64_t timer_resolution = 1;
static volatile uint64_t timer_ticks;

int timer_init(uint64_t resolution) {
    timer_resolution = (uint64_t)resolution;
    timer_ticks = 0;

    return 0;
}

void timer_tick() { timer_ticks++; }

uint64_t timer_get_ticks() { return timer_ticks; }

uint64_t timer_get_resolution() { return timer_resolution; }

uint64_t uptimer_ms() { return (timer_ticks * 1000) / timer_resolution; }

void usleep(time_t us) {
    tid_t tid;
    if (sched_current_thread(&tid) == 0) {
        /* Called from a task, use scheduler to sleep */
        sched_usleep(tid, us);
        sched_yield();
        return;
    }

    uint64_t ticks_start = (timer_ticks * 1000) / timer_resolution * 1000 + us;
    while (ticks_start > (timer_ticks * 1000) / timer_resolution * 1000)
        ;
}
