/*
 * Copyright PolySat, California Polytechnic State University, San Luis Obispo. cubesat@calpoly.edu
 * This file is part of libproc, a PolySat library.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _EVENT_TIMER_H_
#define _EVENT_TIMER_H_

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VIRT_CLK_ENABLED 1
#define VIRT_CLK_DISABLED 0

#define VIRT_CLK_STOLEN 2
#define VIRT_CLK_PAUSED 1
#define VIRT_CLK_ACTIVE 0


struct EventTimer;
/**
 * Function pointer callback to peform actual blocking operation (poll,
 * select, etc).  This allows separation of the underlying event API
 * from the EventTimer API.
 */
typedef int (*ET_block_cb)(struct EventTimer *et, struct timeval *nextAwake,
		void *arg);
/**
 * The EventTimer abstraction allows the libproc event loop to
 * be agnostic of time. This allows virtualization of the event
 * loop so programs can be run in accelerated time.
 *
 * There are two EventTimer implementations provided, the default
 * EventTimer and the virtual EventTimer. The default EventTimer produces
 * the behavior expected of libproc event loop. The virtual EventTimer
 * immediately executes timed events and maintains a virtual time that
 * can be accessed using EVT_get_gmt_time or EVT_get_monotonic_time. Execution
 * of fd events is the same as the default EventTimer.
 */
struct EventTimer {
   /**
    * Block the event loop until either an event occurs or the time stored 
    * in nextAwake elapses.
    */
   int (*block)(struct EventTimer *et, struct timeval *nextAwake,
         int pauseWhileBlocked, ET_block_cb blockcb, void *arg);

   /**
    * Return the current gmt time.
    */
   int (*get_gmt_time)(struct EventTimer *et, struct timeval *tv);

   /**
    * Return the current monotonic time.
    */
   int (*get_monotonic_time)(struct EventTimer *et, struct timeval *tv);

   /**
    * Cleanup the event timer.
    */
   void (*cleanup)(struct EventTimer *et);

   /**
    * Increment virtual clock time.
    *
    * @param clk a Virtual EventTimer object.
    * @param time to increment the clock by.
    */
   void (*virt_inc_time)(struct EventTimer *et, struct timeval *time);

   /**
    * Set virtual clock time.
    *
    * @param clk a Virtual EventTimer object.
    * @param time to set the clock to.
    */
   void (*virt_set_time)(struct EventTimer *et, struct timeval *time);

   /**
    * Get virtual clock time. Advanced usage only. Most applications 
    * should use 'EVT_get_gmt_time' instead.
    *
    * @param clk a Virtual EventTimer object.
    */
   int (*virt_get_time)(struct EventTimer *et, struct timeval *time);

   /**
    * Set Virtual EventTimer pause state
    *
    * @param clk a Virtual EventTimer object.
    * @param pauseState ET_virt_PAUSED or CLK_ACTIVE.
    */
   void (*virt_set_pause)(struct EventTimer *et, char pauseState);

   /**
    * Get Virtual EventTimer pause state
    *
    * @param clk a Virtual EventTimer object.
    *
    * @return VIRT_CLK_PAUSED or VIRT_CLK_ACTIVE.
    */
   char (*virt_get_pause)(struct EventTimer *et);
};

/**
 * Create a default event manager. Standard libproc execution.
 *
 * @param The initial clock time.
 */
struct EventTimer *ET_default_init();

/**
 * Create an event timer for use with the debugger. Standard libproc execution.
 */
struct EventTimer *ET_rtdebug_init();

/**
 * Create a virtual event manager, which executes timed events as fast as possible.
 *
 * @param The initial clock time.
 */
struct EventTimer *ET_virt_init(struct timeval *initTime);

/**
 * Create a global virtual event manager, which executes timed events as fast
 *  as possible and synchronizes this virtual clock between processes on the
 *  same system.
 *
 * @param state_file The file to use for synchronizing clock state.
 * @param pause_mode The initial pause mode of the timer
 */
struct EventTimer *ET_gvirt_init(const char *state_file, char pause_mode);


#ifdef __cplusplus
}
#endif
#endif
