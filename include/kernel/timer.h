#ifndef _INARI_TIME_H
#define _INARI_TIME_H

#include <misc/types.h>

typedef int64_t time_t;

/* Sets the hardware timer resolution.
 *A value of 1000 would mean the timer ticks 1000 times a second, or every 1 millisecond.
 *
 * Also resets the timer logic.
 */
int timer_init(size_t resolution);

/* A single tick sent from the hardware timer */
void timer_tick();

/* Returns current count of ticks */
size_t timer_get_ticks();

/* Returns current timer resolution (ticks per second) */
size_t timer_get_resolution();

/* Returns current kernel uptime in miliseconds */
size_t uptimer_ms();

/* Sleep in microseconds */
void usleep(size_t us);

#endif