/*
 * wallclock.h
 *
 * Created: 7/29/2013 4:02:40 PM
 *  Author: clarkson
 */

#ifndef WALLCLOCK_H_
#define WALLCLOCK_H_

#include "static_assert.h"

#define WALLCLOCK_TICKS_PER_SEC CPU_HZ
typedef uint64_t wallclock_t;

STATIC_ASSERT(sizeof(wallclock_t) == 8, wallclock_t_is_wrong_size);

typedef struct
{
  wallclock_t end_time;
  bool running;
} walltimer_t;

extern volatile wallclock_t wallclock_base;

__always_inline static wallclock_t wallclock(void)
{
  return wallclock_base + (wallclock_t)Get_system_register(AVR32_COUNT);
}

__always_inline static void walltimer_schedule(wallclock_t end_time, walltimer_t *timer)
{
  timer->end_time = end_time;
  timer->running = true;
}

__always_inline static void walltimer_start(unsigned long delay, walltimer_t *timer)
{
  timer->end_time = wallclock() + (wallclock_t)delay;
  timer->running = true;
}

__always_inline static bool walltimer_elapsed(walltimer_t *timer)
{
  if (timer->running && wallclock() > timer->end_time)
  {
    timer->running = false;
    return true;
  }

  return false;
}

__always_inline static void walltimer_restart(unsigned long delay, walltimer_t *timer)
{
  timer->end_time += (wallclock_t)delay;
  timer->running = true;
}

__always_inline static void wallclock_delay(unsigned long delay)
{
  wallclock_t until = wallclock() + (wallclock_t)delay;
  while (wallclock() < until)
    ;
}

#endif /* WALLCLOCK_H_ */