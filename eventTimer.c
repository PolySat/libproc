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

#include "events.h"
#include "eventTimer.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>

static void et_virt_inc_time(struct EventTimer *et, struct timeval *time);
static void et_virt_set_time(struct EventTimer *et, struct timeval *time);
static int et_virt_get_time(struct EventTimer *et, struct timeval *time);
static void et_virt_set_pause(struct EventTimer *et, char pauseState);
static char et_virt_get_pause(struct EventTimer *et);

int ET_default_block(struct EventTimer *et, struct timeval *nextAwake,
      int pauseWhileBlocking, ET_block_cb blockcb, void *arg)
{
   struct timeval diffTime, curTime, *blockTime = NULL;
   
   et->get_monotonic_time(et, &curTime);
   
   // Set amount of time to block on select.
   // If no events, block indefinetly. 
   if (nextAwake) {
      timersub(nextAwake, &curTime, &diffTime);
      if (diffTime.tv_sec < 0 || diffTime.tv_usec < 0) {
         memset(&diffTime, 0, sizeof(struct timeval));
      }
      blockTime = &diffTime;
   }

   return blockcb(et, blockTime, arg);
}

int ET_default_gmt(struct EventTimer *et, struct timeval *tv)
{
   gettimeofday(tv, NULL);
   return 0;
}

int ET_default_monotonic(struct EventTimer *et, struct timeval *tv)
{
   int res;

   #ifdef __APPLE__

   res = 0;
   gettimeofday(tv, NULL);

   #else

   struct timespec tp;
   res = clock_gettime(CLOCK_MONOTONIC, &tp);
   tv->tv_sec = tp.tv_sec;
   tv->tv_usec = (tp.tv_nsec + 500 ) / 1000;

   #endif

   return res;
}

void ET_default_cleanup(struct EventTimer *et)
{
   if (et)
      free(et);
}

struct EventTimer *ET_default_init()
{
   struct EventTimer *et;

   et = malloc(sizeof(struct EventTimer));
   if (!et)
      return NULL;
   memset(et, 0, sizeof(struct EventTimer));

   et->block = &ET_default_block;
   et->get_gmt_time = &ET_default_gmt;
   et->get_monotonic_time = &ET_default_monotonic;
   et->cleanup = &ET_default_cleanup;

   return et;
}

struct RTDebugEventTimer {
   struct EventTimer et;
   struct timeval offset;
};

int ET_rtdebug_block(struct EventTimer *timer, struct timeval *nextAwake,
      int pauseWhileBlocking, ET_block_cb blockcb, void *arg)
{
   struct RTDebugEventTimer *et = (struct RTDebugEventTimer *)timer;
   struct timeval diffTime, curTime, endTime, *blockTime = NULL;
   int res;
   
   et->et.get_monotonic_time(&et->et, &curTime);
   
   // Set amount of time to block on select.
   // If no events, block indefinetly. 
   if (nextAwake) {
      timersub(nextAwake, &curTime, &diffTime);
      if (diffTime.tv_sec < 0 || diffTime.tv_usec < 0) {
         memset(&diffTime, 0, sizeof(struct timeval));
      }
      blockTime = &diffTime;
   }

   res = blockcb(&et->et, blockTime, arg);

   if (pauseWhileBlocking) {
      et->et.get_monotonic_time(&et->et, &endTime);
      timersub(&endTime, &curTime, &diffTime);
      if (diffTime.tv_sec < 0 || diffTime.tv_usec < 0) {
         memset(&diffTime, 0, sizeof(struct timeval));
      }
      curTime = et->offset;
      timeradd(&diffTime, &curTime, &et->offset);
   }

   return res;
}

int ET_rtdebug_gmt(struct EventTimer *arg, struct timeval *tv)
{
   struct RTDebugEventTimer *et = (struct RTDebugEventTimer *)arg;
   struct timeval now;

   gettimeofday(&now, NULL);
   timersub(&now, &et->offset, tv);

   return 0;
}

int ET_rtdebug_monotonic(struct EventTimer *arg, struct timeval *tv)
{
   int res;
   struct RTDebugEventTimer *et = (struct RTDebugEventTimer *)arg;
   struct timeval now;

   #ifdef __APPLE__

   res = 0;
   // WARNING: Not a monotonic clock, simply a stub for Apple devices.
   gettimeofday(&now, NULL);

   #else

   struct timespec tp;
   res = clock_gettime(CLOCK_MONOTONIC, &tp);
   now.tv_sec = tp.tv_sec;
   now.tv_usec = (tp.tv_nsec + 500 ) / 1000;

   #endif

   timersub(&now, &et->offset, tv);

   return res;
}

void ET_rtdebug_cleanup(struct EventTimer *arg)
{
   struct RTDebugEventTimer *et = (struct RTDebugEventTimer *)arg;

   if (et)
      free(et);
}

struct EventTimer *ET_rtdebug_init()
{
   struct RTDebugEventTimer *et;

   et = malloc(sizeof(struct RTDebugEventTimer));
   if (!et)
      return NULL;
   memset(et, 0, sizeof(struct RTDebugEventTimer));

   et->et.block = &ET_rtdebug_block;
   et->et.get_gmt_time = &ET_rtdebug_gmt;
   et->et.get_monotonic_time = &ET_rtdebug_monotonic;
   et->et.cleanup = &ET_rtdebug_cleanup;

   return &et->et;
}

struct VirtualEventTimer {
   struct EventTimer et;
   struct timeval time;
   char paused;
};

int ET_virt_block(struct EventTimer *et, struct timeval *nextAwake,
      int pauseWhileBlocking, ET_block_cb blockcb, void *arg)
{
   struct timeval diffTime, curTime, *blockTime = NULL;
   
   // Set amount of time to block on select.
   // Don't advance the virtual clock when time is paused while blocking
   if (nextAwake && pauseWhileBlocking) {
      et->get_monotonic_time(et, &curTime);
      timersub(nextAwake, &curTime, &diffTime);
      if (diffTime.tv_sec < 0 || diffTime.tv_usec < 0) {
         memset(&diffTime, 0, sizeof(struct timeval));
      }
      blockTime = &diffTime;
   }
   // Block on select indefinetly if virtual clock is paused
   else if (nextAwake && et_virt_get_pause(et) == VIRT_CLK_ACTIVE) {
      et_virt_set_time(et, nextAwake);
      // If virt clk is active, don't block on select.
      memset(&diffTime, 0, sizeof(struct timeval));
      blockTime = &diffTime;
   }

   return blockcb(et, blockTime, arg);
}

void ET_virt_cleanup(struct EventTimer *et)
{
   struct VirtualEventTimer *vet = (struct VirtualEventTimer *)et;
   
   if (vet)
      free(vet);
}

struct EventTimer *ET_virt_init(struct timeval *initTime)
{
   struct VirtualEventTimer *et;

   et = malloc(sizeof(struct VirtualEventTimer));
   if (!et)
      return NULL;
   memset(et, 0, sizeof(struct VirtualEventTimer));

   et->time = *initTime;
   et->paused = VIRT_CLK_ACTIVE;
   et->et.block = &ET_virt_block;
   et->et.get_gmt_time = &et_virt_get_time;
   et->et.get_monotonic_time = &et_virt_get_time;
   et->et.cleanup = &ET_virt_cleanup;
   et->et.virt_inc_time = &et_virt_inc_time;
   et->et.virt_set_time = &et_virt_set_time;
   et->et.virt_get_time = &et_virt_get_time;
   et->et.virt_set_pause = &et_virt_set_pause;
   et->et.virt_get_pause = &et_virt_get_pause;

   return (struct EventTimer *)et;
}

/**
 * Increment virtual clock time.
 *
 * @param clk a Virtual EventTimer object.
 * @param time to increment the clock by.
 */
void et_virt_inc_time(struct EventTimer *et, struct timeval *time)
{
   struct VirtualEventTimer *clk = (struct VirtualEventTimer *)et;

   assert(et);
   
   timeradd(time, &clk->time, &clk->time);
}

/**
 * Set virtual clock time.
 *
 * @param clk a Virtual EventTimer object.
 * @param time to set the clock to.
 */
void et_virt_set_time(struct EventTimer *et, struct timeval *time)
{
   struct VirtualEventTimer *clk = (struct VirtualEventTimer *)et;
   
   assert(et);

   clk->time = *time;
}

/**
 * Get virtual clock time. Advanced usage only. Most applications 
 * should use 'EVT_get_gmt_time' instead.
 *
 * @param clk a Virtual EventTimer object.
 */
int et_virt_get_time(struct EventTimer *et, struct timeval *time)
{
   struct VirtualEventTimer *clk = (struct VirtualEventTimer *)et;

   assert(et);

   *time = clk->time;
   return 0;
}

/**
 * Set Virtual EventTimer pause state
 *
 * @param clk a Virtual EventTimer object.
 * @param pauseState ET_virt_PAUSED or CLK_ACTIVE.
 */
void et_virt_set_pause(struct EventTimer *et, char pauseState)
{
   struct VirtualEventTimer *clk = (struct VirtualEventTimer *)et;

   assert(et);

   if (pauseState != VIRT_CLK_STOLEN)
      clk->paused = pauseState;
}

/**
 * Get Virtual EventTimer pause state
 *
 * @param clk a Virtual EventTimer object.
 *
 * @return VIRT_CLK_PAUSED or VIRT_CLK_ACTIVE.
 */
char et_virt_get_pause(struct EventTimer *et)
{
   struct VirtualEventTimer *clk = (struct VirtualEventTimer *)et;
   
   assert(et);

   return clk->paused;
}
