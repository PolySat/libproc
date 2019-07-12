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
#include <polysat3/polysat.h>
#include <polysat3/cmd-pkt.h>
#include <polysat3/cmd.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include "test_schema.h"

#define BUF_LEN 2048


struct ProcessData *gProc = NULL;
struct timed_multi_st{
   struct ProcessData *gProc;
   uint32_t command_num;
};

static void status_cmd_handler(struct ProcessData *proc,
      struct IPC_Command *cmd,
      struct sockaddr_in *fromAddr, void *arg, int fd);
static void params_cmd_handler(struct ProcessData *proc,
      struct IPC_Command *cmd,
      struct sockaddr_in *fromAddr, void *arg, int fd);

//maps generated command numbers to c functions
struct XDR_CommandHandlers handlers[] = {
   { IPC_CMDS_STATUS, &status_cmd_handler, NULL},
   { IPC_TEST_COMMANDS_PTEST, &params_cmd_handler, NULL},
   { 0, NULL, NULL }
};

static void status_cmd_handler(struct ProcessData *proc, struct IPC_Command *cmd,
      struct sockaddr_in *fromAddr, void *arg, int fd)
{
    //status struct to send back
   struct IPC_TEST_Status status;

   status.foo = 123;
   status.bar = 464;
    
    //pass enum to associate with struct being sent
   IPC_response(proc, cmd, IPC_TEST_DATA_TYPES_STATUS, &status, fromAddr);
   printf("Status command!!\n");
}

static void params_cmd_handler(struct ProcessData *proc, struct IPC_Command *cmd,
      struct sockaddr_in *fromAddr, void *arg, int fd)
{
   struct IPC_TEST_PTest *params = (struct IPC_TEST_PTest*)cmd->parameters.data;

   if (params)
      printf("v1=%d, v2=%d, v3=%d, v4=%d\n", params->val1, params->val2, params->val3, params->val4);

   IPC_response(proc, cmd, IPC_TYPES_VOID, NULL, fromAddr);
   printf("Params command!!\n");
}
/*
static void telem_cmd_handler(struct ProcessData *proc,
      struct IPC_Command *cmd,
      struct sockaddr_in *fromAddr, void *arg, int fd);
//maps generated command numbers to c functions
struct XDR_CommandHandlers handlers[] = {
   { IPC_TEST_COMMANDS_TELEM, &telem_cmd_handler, NULL},
   { 0, NULL, NULL }
};

static void telem_cmd_handler(struct ProcessData *proc, struct IPC_Command *cmd,
      struct sockaddr_in *fromAddr, void *arg, int fd)
{
   printf("telem command!!\n");
}
*/
// Simple SIGINT handler example
int sigint_handler(int signum, void *arg)
{
   DBG("SIGINT handler!\n");
   EVT_exit_loop(PROC_evt(arg));
   return EVENT_KEEP;
}
int timed_multi(void *arg){
   struct timed_multi_st *arg_st = (struct timed_multi_st *)arg;

   IPC_multi_command(arg_st->gProc, arg_st->command_num,NULL,0,NULL,NULL,IPC_CB_TYPE_RAW,3000);
   return EVENT_KEEP;

}

int main(int argc, char *argv[])
{

   const char *execName = NULL;
   struct CMD_XDRCommandInfo *command = NULL;
   struct timed_multi_st *mult_arg;
   uint32_t command_num;
   if(NULL == (mult_arg = malloc(sizeof(struct timed_multi_st)))){
      perror("malloc multicast timed event arg struct\n");
   }
   gProc = PROC_init_xdr("test1", WD_DISABLED, handlers);

   execName = "test-status";
   command = CMD_xdr_cmd_by_name(execName);
   if(!command){

      printf("command %s not found\n", execName);
      return -1;
   }

   command_num = command->command;
   IPC_multi_command(gProc, command_num,NULL,0,NULL,NULL,IPC_CB_TYPE_RAW,3000);
   printf("first cmd executed successfully\n");
   mult_arg->gProc = gProc;
   mult_arg->command_num = command_num;

   // Add a signal handler call back for SIGINT signal
   PROC_signal(gProc, SIGINT, &sigint_handler, gProc);
   EVT_sched_add(PROC_evt(gProc), EVT_ms2tv(1000),&timed_multi,mult_arg);

   EVT_start_loop(PROC_evt(gProc));
   free(mult_arg);

   return 0;
}
