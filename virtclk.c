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
#include "virtclk.h"
#include "events.h"
#include "proclib.h"
#include <stdlib.h>
#include <string.h>

/**
 * Init a VirtualClkState object.
 *
 * @param clk a VirtualClkState object.
 * @param time to increment the clock by.
 */
struct VirtClkState *CLK_init(struct timeval *initTime)
{
   struct VirtClkState *res;

   res = malloc(sizeof(struct VirtClkState));
   if (!res) {
      return NULL;
   }
   memset(res, 0, sizeof(struct VirtClkState));
   
   res->time = *initTime;
   res->paused = CLK_ACTIVE;
   return res;
}

/**
 * Increment virtual clock time.
 *
 * @param clk a VirtualClkState object.
 * @param time to increment the clock by.
 */
void CLK_inc_time(struct VirtClkState *clk, struct timeval *time)
{
   if (!clk) {
      return;
   }

   timeradd(time, &clk->time, &clk->time);
}

/**
 * Set virtual clock time.
 *
 * @param clk a VirtualClkState object.
 * @param time to set the clock to.
 */
void CLK_set_time(struct VirtClkState *clk, struct timeval *time)
{
   if (!clk) {
      return;
   }

   clk->time = *time;
}

/**
 * Get virtual clock time. Advanced usage only. Most applications 
 * should use 'EVT_get_gmt_time' instead.
 *
 * @param clk a VirtualClkState object.
 */
struct timeval CLK_get_time(struct VirtClkState *clk)
{
   return clk->time;
}

/**
 * Set VirtualClkState pause state
 *
 * @param clk a VirtualClkState object.
 * @param pauseState CLK_PAUSED or CLK_ACTIVE.
 */
void CLK_set_pause(struct VirtClkState *clk, char pauseState)
{
   if (!clk) {
      return;
   }

   clk->paused = pauseState;
}

/**
 * Get VirtualClkState pause state
 *
 * @param clk a VirtualClkState object.
 *
 * @return CLK_PAUSED or CLK_ACTIVE.
 */
char CLK_get_pause(struct VirtClkState *clk)
{
   if (!clk) {
      return -1;
   }

   return clk->paused;
}

/**
 * Update the virtual clock to a timeval if the virtual clock isn't
 * paused.
 *
 * @param clk a VirtualClkState object.
 * @param nextAwake a timeval pointer representing the time the clock should be set to.
 *
 * @return the pause state.
 */
int CLK_follow_event(struct VirtClkState *clk, struct timeval *nextAwake)
{
   if (!clk) {
      return CLK_ACTIVE;
   }

   if (CLK_get_pause(clk) != CLK_PAUSED) {
      CLK_set_time(clk, nextAwake);
      return CLK_ACTIVE;
   }

   return CLK_PAUSED;
}

/**
 * Cleanup virtual clk state.
 *
 * @param clk a VirtualClkState object
 */
void CLK_cleanup(struct VirtClkState *clk)
{
   if (!clk) {
      return;
   }

   free(clk);
}

