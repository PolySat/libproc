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
#include <polysat3/polysat.h>
#include <polysat3/cmd-pkt.h>
#include "test_schema.h"

struct ProcessData *gProc = NULL;

static void mcast_cmd_handler(struct ProcessData *proc, struct IPC_Command *cmd,
      struct sockaddr_in *fromAddr, void *arg, int fd)
{
   struct IPC_TEST_Mcast *params = (struct IPC_TEST_Mcast*)cmd->parameters.data;

   if (params)
      printf("vl1=%d, vl2=%d\n", params->val1, params->val2);
}

// Simple SIGINT handler example
int sigint_handler(int signum, void *arg)
{
   DBG("SIGINT handler!\n");
   EVT_exit_loop(PROC_evt(arg));
   return EVENT_KEEP;
}

int main(int argc, char *argv[])
{
   gProc = PROC_init_xdr(NULL, WD_DISABLED, NULL);

   // Add a signal handler call back for SIGINT signal
   DBG_setLevel(DBG_LEVEL_ALL);
   PROC_signal(gProc, SIGINT, &sigint_handler, gProc);
   PROC_set_multicast_xdr_handler(gProc, "test1", IPC_TEST_COMMANDS_MCAST_TEST,
         &mcast_cmd_handler, gProc);

   EVT_start_loop(PROC_evt(gProc));

   return 0;
}
