#include <kernel/inari.h>
#include <kernel/time.h>
#include <kernel/sched/sched.h>

static size_t time_curr_resolution = 1;
static size_t time_ticks;

void time_resolution(size_t resolution)
{
    time_curr_resolution = resolution;
    time_ticks = 0;
}

void time_tick()
{
    time_ticks++;
}

size_t time_get_ticks()
{
    return time_ticks;
}

size_t uptime_ms()
{
    return (time_ticks * 1000) / time_curr_resolution;
}

void usleep(size_t us)
{
    tid_t tid;
    // if (sched_current_task(&tid) == 0)
    // {
    //     /* Called from a task, use scheduler to sleep */
    //     sched_sleep(tid, us);
    //     return;
    // }

    // (4970 * 1000 * 1000) / 4970

    size_t ticks_start = (time_ticks * 1000) / time_curr_resolution * 1000 + us;
    while (ticks_start > (time_ticks * 1000) / time_curr_resolution * 1000)
        ;
}