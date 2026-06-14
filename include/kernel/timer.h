#ifndef _INARI_TIME_H
#define _INARI_TIME_H

#include <misc/types.h>

#ifndef time_t
typedef uint64_t time_t;
#endif

/* Sets the hardware timer resolution.
 *A value of 1000 would mean the timer ticks 1000 times a second, or every 1 millisecond.
 *
 * Also resets the timer logic.
 */
int timer_init(uint64_t resolution);

/* A single tick sent from the hardware timer */
void timer_tick();

/* Returns current count of ticks */
uint64_t timer_get_ticks();

/* Returns current timer resolution (ticks per second) */
uint64_t timer_get_resolution();

/* Returns current kernel uptime in microseconds */
uint64_t uptime_us();

/* Sleep in microseconds */
void timer_usleep(time_t us);

#endif
