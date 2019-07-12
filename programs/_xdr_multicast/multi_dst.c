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
#include <polysat/polysat.h>
#include <string.h>
#include <signal.h>

#define BUF_LEN 2048

struct ProcessData *gProc = NULL;

// Simple SIGINT handler example
int sigint_handler(int signum, void *arg)
{
   DBG("SIGINT handler!\n");
   EVT_exit_loop(PROC_evt(arg));
   return EVENT_KEEP;
}

static void mcast_handler(void *arg, int socket, unsigned char cmd,
   void *data, size_t dataLen, struct sockaddr_in *fromAddr)
{
   printf("Multicast CMD: %d\n", cmd);
}

int main(int argc, char *argv[])
{
   gProc = PROC_init("test2",WD_DISABLED);

   // Add a signal handler call back for SIGINT signal
   PROC_signal(gProc, SIGINT, &sigint_handler, gProc);

   PROC_set_multicast_handler(gProc, "test1", -1, &mcast_handler, gProc);
   PROC_set_multicast_handler(gProc, "test1", -1, &mcast_handler, gProc);
   PROC_set_multicast_handler(gProc, "test1", 10, &mcast_handler, gProc);
   PROC_set_multicast_handler(gProc, "test1", 11, &mcast_handler, gProc);

   EVT_start_loop(PROC_evt(gProc));

   return 0;
}
