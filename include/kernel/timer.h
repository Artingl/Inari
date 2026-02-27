#ifndef _INARI_TIME_H
#define _INARI_TIME_H

#include <misc/types.h>

/* Sets the hardware timer resolution.
 *A value of 1000 would mean the timer ticks 1000 times a second, or every 1 millisecond.
 *
 * Also resets the timer logic.
 */
extern int timer_init(size_t resolution);

/* A single tick sent from the hardware timer */
extern void timer_tick();

/* Returns current count of ticks */
extern size_t timer_get_ticks();

/* Returns current timer resolution (ticks per second) */
extern size_t timer_get_resolution();

/* Returns current kernel uptime in miliseconds */
extern size_t uptimer_ms();

/* Sleep in microseconds */
extern void usleep(size_t us);

#endif