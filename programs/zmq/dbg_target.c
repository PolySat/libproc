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

/**
 * Test code for process command handling.
 *
 * Don't input any lines over BUF_LEN or a buffer overrun will occur.
 *
 * @author Greg Eddington
 */
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "zmqlite.h"
#include "events.h"
#include "debug.h"
#include "proclib.h"
#include "ipc.h"

struct ProcessData *gProc = NULL;

// Simple SIGINT handler example
int sigint_handler(int signum, void *arg)
{
   DBG("SIGINT handler!\n");
   EVT_exit_loop(PROC_evt(arg));
   return EVENT_KEEP;
}

int poke_cb(void *arg)
{
   return EVENT_KEEP;
}

int main(int argc, char *argv[])
{
   gProc = PROC_init("test1", WD_DISABLED);

   // Add a signal handler call back for SIGINT signal
   PROC_signal(gProc, SIGINT, &sigint_handler, gProc);
   EVT_sched_add(PROC_evt(gProc), EVT_ms2tv(2000), poke_cb, NULL);
   EVT_set_initial_debugger_state(PROC_evt(gProc), EDBG_STOPPED);

   EVT_start_loop(PROC_evt(gProc));

   return 0;
}
