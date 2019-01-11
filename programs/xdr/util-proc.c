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
#include <polysat/zmqlite.h>
#include <polysat/events.h>
#include <polysat/debug.h>
#include <polysat/proclib.h>
#include <polysat/ipc.h>
#include <polysat/cmd_schema.h>
#include "../src/adcs_schema.h"

struct ProcessData *gProc = NULL;

void command_cb(struct ProcessData *proc, int timeout, void *arg,
      char *resp_buff, size_t resp_len, enum IPC_CB_TYPE cb_type)
{

   if (!timeout)
      printf("Response!!\n");
   else
      printf("Timeout!\n");
   if (cb_type == IPC_CB_TYPE_RAW)
      CMD_print_response(proc, timeout, NULL, resp_buff, resp_len, cb_type);
   else if (resp_buff) {
      struct IPC_Response *resp = (struct IPC_Response*)resp_buff;
      struct IPC_ADCS_CTRL_BDOT_StateData *dbg =
                   (struct IPC_ADCS_CTRL_BDOT_StateData*)resp->data.data;

      if (dbg)
         printf("Debug: %d\n", dbg->reading_sec);
   }

   EVT_exit_loop(PROC_evt(proc));
}

void send_bdot_request(void)
{
   struct sockaddr_in dest;

   // configure the socket address with the looked up port
   memset((char*)&dest, 0, sizeof(dest));
   // Select IPv4 Socket type
   dest.sin_family = AF_INET;
   // Look up the port corresponding to the name
   dest.sin_port = htons(socket_get_addr_by_name("adcs"));
   // Use localhost ip as source of message
   dest.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

   IPC_command(gProc, IPC_ADCS_CTRL_BDOT_CMDS_DEBUG, NULL, 0, dest,
         &command_cb, NULL, IPC_CB_TYPE_RAW, 3000);
}

int send_it(void*foo)
{
   send_bdot_request();
   return EVENT_REMOVE;
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
   IPC_ADCS_forcelink();
   gProc = PROC_init("test1", WD_DISABLED);

   // Add a signal handler call back for SIGINT signal
   DBG_setLevel(DBG_LEVEL_ALL);
   PROC_signal(gProc, SIGINT, &sigint_handler, gProc);
   EVT_sched_add(PROC_evt(gProc), EVT_ms2tv(1), send_it, NULL);

   EVT_start_loop(PROC_evt(gProc));

   return 0;
}
