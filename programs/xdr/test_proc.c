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
#include "cmd_schema.h"
#include "test_schema.h"

struct ProcessData *gProc = NULL;

static void status_cmd_handler(struct ProcessData *proc,
      struct IPC_Command *cmd,
      struct sockaddr_in *fromAddr, void *arg, int fd);
static void params_cmd_handler(struct ProcessData *proc,
      struct IPC_Command *cmd,
      struct sockaddr_in *fromAddr, void *arg, int fd);

struct XDR_CommandHandlers handlers[] = {
   { IPC_COMMANDS_STATUS, &status_cmd_handler, NULL},
   { IPC_TEST_COMMANDS_PTEST, &params_cmd_handler, NULL},
   { 0, NULL, NULL }
};

static void status_cmd_handler(struct ProcessData *proc, struct IPC_Command *cmd,
      struct sockaddr_in *fromAddr, void *arg, int fd)
{
   struct IPC_TEST_Status status;

   status.foo = 123;
   status.bar = 464;
   IPC_response(proc, cmd, IPC_TEST_DATA_TYPES_STATUS, &status, fromAddr);
   printf("Status command!!\n");
}

static void params_cmd_handler(struct ProcessData *proc, struct IPC_Command *cmd,
      struct sockaddr_in *fromAddr, void *arg, int fd)
{
   struct IPC_TEST_PTest *params = (struct IPC_TEST_PTest*)cmd->parameters.data;

   if (params)
      printf("v1=%d, v2=%d, v3=%d, v4=%d\n", params->val1, params->val2, params->val3, params->val4);

   IPC_response(proc, cmd, IPC_DATA_TYPES_VOID, NULL, fromAddr);
   printf("Params command!!\n");
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
   gProc = PROC_init_xdr("test1", WD_DISABLED, handlers);

   // Add a signal handler call back for SIGINT signal
   DBG_setLevel(DBG_LEVEL_ALL);
   PROC_signal(gProc, SIGINT, &sigint_handler, gProc);

   EVT_start_loop(PROC_evt(gProc));

   return 0;
}
