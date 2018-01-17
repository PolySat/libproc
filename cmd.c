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
 * @file cmd.c Command Handler Source
 *
 * Command handler library component implementation source file.
 *
 * @author Greg Manyak
 * @author John Bellardo
 */
#include <dlfcn.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "config.h"
#include <string.h>
#include "proclib.h"
#include "ipc.h"
#include "cmd.h"
#include "debug.h"

/// Value in the PROT element in the CMD structure that indicates protected cmd
#define CMD_PROTECTED 1

// Code to handle multicast packet management
struct MulticastCommand {
   int cmdNum;
   MCAST_handler_t callback;
   void *callbackParam;

   struct MulticastCommand *next;
};

struct McastCommandState {
   struct in_addr srcAddr;
   uint16_t port;
   int fd;
   struct MulticastCommand *cmds;
   struct McastCommandState *next;
};

static int multicast_cmd_handler_cb(int socket, char type, void * arg)
{
   unsigned char data[MAX_IP_PACKET_SIZE];
   struct MulticastCommand *cmd = NULL;
   struct McastCommandState *state = (struct McastCommandState*)arg;
   size_t dataLen;
   struct sockaddr_in src;
   data[0] = 0;

   if (!state)
      return EVENT_KEEP;

   // should only be read events, but make sure
   if (type == EVENT_FD_READ) {
      // read from the socket to get the command and it's data
      dataLen = socket_read(socket, data, MAX_IP_PACKET_SIZE, &src);

      // make sure something was actually read
      if (dataLen > 0) {
         DBG_print(DBG_LEVEL_INFO, "MCast Received command 0x%02x", *data);

         for (cmd = state->cmds; cmd; cmd = cmd->next) {
            if (cmd->cmdNum < 0 || cmd->cmdNum == *data)
               cmd->callback(cmd->callbackParam, socket, *data, &data[1],
                  dataLen - 1, &src);
         }
      }
   }

   return EVENT_KEEP;
}

static struct McastCommandState *find_mcast_state(struct CommandCbArg *st,
   struct in_addr addr, uint16_t port)
{
    struct McastCommandState *curr;

    port = htons(port);
    for (curr = st->mcast; curr; curr = curr->next)
       if (curr->port == port && curr->srcAddr.s_addr == addr.s_addr)
          return curr;

    return NULL;
}

void cmd_set_multicast_handler(struct CommandCbArg *st,
   struct EventState *evt_loop, const char *service, int cmdNum,
   MCAST_handler_t handler, void *arg)
{
   struct MulticastCommand *cmd;
   struct in_addr addr = socket_multicast_addr_by_name(service);
   uint16_t port = socket_multicast_port_by_name(service);
   struct McastCommandState *state;
   struct ip_mreq mreq;

   if (!addr.s_addr || !port)
      return;

   state = find_mcast_state(st, addr, port);
   if (!state) {
      state = (struct McastCommandState*)malloc(sizeof(*state));
      memset(state, 0, sizeof(*state));

      state->srcAddr = addr;
      state->port = htons(port);
      state->fd = socket_init(port);
      if (state->fd <= 0) {
         free(state);
         return;
      }

      // Join multicast group
      mreq.imr_interface.s_addr = htonl(INADDR_ANY);
      mreq.imr_multiaddr.s_addr = addr.s_addr;
      if (setsockopt(state->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
		 sizeof(struct ip_mreq)) == -1) {
         ERR_REPORT(DBG_LEVEL_WARN, "Failed to join multicast group for %s\n",
            service);
         return;
      }

      EVT_fd_add(evt_loop, state->fd, EVENT_FD_READ, multicast_cmd_handler_cb,
                     state);
      state->next = st->mcast;
      st->mcast = state;
   }

   cmd = (struct MulticastCommand*)malloc(sizeof(*cmd));
   memset(cmd, 0, sizeof(*cmd));

   cmd->cmdNum = cmdNum;
   cmd->callback = handler;
   cmd->callbackParam = arg;

   cmd->next = state->cmds;
   state->cmds = cmd;
}

void cmd_remove_multicast_handler(struct CommandCbArg *st,
   const char *service, int cmdNum, struct EventState *evt_loop)
{
   struct MulticastCommand **itr, *cmd;
   struct McastCommandState **st_itr, *state;
   struct ip_mreq mreq;
   struct in_addr addr = socket_multicast_addr_by_name(service);
   uint16_t port = socket_multicast_port_by_name(service);

   state = find_mcast_state(st, addr, port);
   if (!state)
      return;

   // Remove all matching entries
   itr = &state->cmds;
   while (itr && *itr) {
      cmd = *itr;
      if ( cmd->cmdNum == cmdNum) {
         *itr = cmd->next;
         free(cmd);
         continue;
      }

      itr = &cmd->next;
   }

   // Clean up the socket if there are no more commands registered
   if (!state->cmds) {
      if (state->fd > 0) {
         EVT_fd_remove(evt_loop, state->fd, EVENT_FD_READ);

         mreq.imr_interface.s_addr = htonl(INADDR_ANY);
         mreq.imr_multiaddr.s_addr = addr.s_addr;
         setsockopt(state->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq,
	                             sizeof(struct ip_mreq));
         close(state->fd);
      }
      state->fd = 0;

      for (st_itr = &st->mcast; st_itr && *st_itr; ) {
         state = *st_itr;
         if (!state->fd) {
            *st_itr = state->next;
            free(state);
         }
         else
            st_itr = &state->next;
      }
   }
}

void cmd_cleanup_cb_state(struct CommandCbArg *st, struct EventState *evt_loop)
{
   struct McastCommandState *state;
   struct MulticastCommand *cmd;
   struct ip_mreq mreq;

   while ((state = st->mcast)) {
      while ((cmd = state->cmds)) {
         state->cmds = cmd->next;
         free(cmd);
      }

      if (state->fd > 0) {
         EVT_fd_remove(evt_loop, state->fd, EVENT_FD_READ);

         mreq.imr_interface.s_addr = htonl(INADDR_ANY);
         mreq.imr_multiaddr.s_addr = state->srcAddr.s_addr;
         setsockopt(state->fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq,
	                             sizeof(struct ip_mreq));
         close(state->fd);
      }

      st->mcast = state->next;
      free(state);
   }
}

// Structure to hold a single command
struct CFG_CommandDesc {
   char *procName;
   char *cmdName;
   uint32_t cmdNum;
   char *funcName;
   CMD_handler_t cmdHandler;

   uint32_t cmdUid, cmdGroup, cmdProt;
};

static void CFG_CommandDesc_free(void *val)
{
   struct CFG_CommandDesc *curr = (struct CFG_CommandDesc *)val;
   if (!curr)
      return;

   if (curr->procName)
      free(curr->procName);

   if (curr->cmdName)
      free(curr->cmdName);

   if (curr->funcName)
      free(curr->funcName);

   free(curr);
}

// Create the CFG object that describes how to parse a single CommandDesc
CFG_NEWOBJ(CommandDesc,
      // Allocate a new instance of the struct
      CFG_MALLOC(struct CFG_CommandDesc),
      CFG_NULL,
      // PROC string value put into the structure with strdup
      CFG_STRDUP("PROC", struct CFG_CommandDesc, procName),
      // NAME string value put into the structure with strdup
      CFG_STRDUP("NAME", struct CFG_CommandDesc, cmdName),
      // FUNC string value put into the structure with strdup
      CFG_STRDUP("FUNC", struct CFG_CommandDesc, funcName),
      // NUM uint32 value put into the structure
      CFG_UINT32("NUM", struct CFG_CommandDesc, cmdNum),
      CFG_UINT32("CMDOID", struct CFG_CommandDesc, cmdUid),
      CFG_UINT32("GRPOID", struct CFG_CommandDesc, cmdGroup),
      CFG_UINT32("PROTECTION", struct CFG_CommandDesc, cmdProt)
);

// Declare the configuration root structure.  In this case it just contains
//  one array.
struct CFG_Root {
   struct CFG_Array cmds;
};

// Create the CFG object that describes how to parse the file root.
CFG_NEWOBJ(Root,
   // Allocate a new instance of the struct
   CFG_MALLOC(struct CFG_Root),
   CFG_NULL,
   // Only one option "CMD", appended to an array
   CFG_OBJARR("CMD", &CFG_OBJNAME(CommandDesc), struct CFG_Root, cmds)
);

void invalidCommand(int socket, unsigned char cmd, void * data, size_t dataLen,
                                                      struct sockaddr_in * src);

// Initialize the command handler
int cmd_handler_init(const char * procName, struct CommandCbArg *cmds)
{
   struct CFG_Root *root = NULL;
   int i;
   char cfgFile[256];
   char statusCmdFound = 0;

   if (procName) {
      sprintf(cfgFile, "./%s.cmd.cfg", procName);

      // Open the configuration file with the commands
      if (!CFG_locateConfigFile(cfgFile)) {
         DBG_print(DBG_LEVEL_FATAL, "No command configuration file found\n");
         return -1;
      }
      // Parse the commands
      root = (struct CFG_Root*) CFG_parseFile(&CFG_OBJNAME(Root));

      DBG_print(DBG_LEVEL_INFO, "%s found config file\n", procName);
   }

   cmds->cmds = (struct Command*)malloc(sizeof(struct Command)*MAX_NUM_CMDS);

   // Initialize array to the "invalid command" function
   for (i = 0; i < MAX_NUM_CMDS; i++) {
      (cmds->cmds + i)->cmd_cb = (CMD_handler_t)invalidCommand;
   }

   // Iterate through the commands
   for (i = 0; root && i < root->cmds.len; i++) {
      // Get the next object from the file
      struct CFG_CommandDesc *cmd =
         (struct CFG_CommandDesc*)root->cmds.data[i];

      // Translate the function name into a function pointer!
      cmd->cmdHandler = (CMD_handler_t)dlsym(RTLD_DEFAULT, cmd->funcName);

      // Check if it actually returned a valid pointer
      if (!cmd->cmdHandler) {
         DBG_print(DBG_LEVEL_WARN, "[%s command file parse error] %s\n",
               procName, dlerror());
      } else {
         DBG_print(DBG_LEVEL_INFO, "%s registered cmd %s [%d]\n", procName,
                                                cmd->funcName, cmd->cmdNum);
         // Check if a status command was found
         statusCmdFound |= (cmd->cmdNum == 0x01);
         // Add command and related params to array
         (cmds->cmds + cmd->cmdNum)->cmd_cb = cmd->cmdHandler;
         (cmds->cmds + cmd->cmdNum)->uid = cmd->cmdUid;
         (cmds->cmds + cmd->cmdNum)->group = cmd->cmdGroup;
         (cmds->cmds + cmd->cmdNum)->prot = cmd->cmdProt;
      }
   }

   if (procName && !statusCmdFound) {
      DBG_print(DBG_LEVEL_WARN, "No status command found -- will not function "
                                "correctly with full system!\n");
   }

   if (root) {
      CFG_freeArray(&root->cmds, &CFG_CommandDesc_free);
      free(root);
   }

   return EXIT_SUCCESS;
}

int cmd_handler_cb(int socket, char type, void * arg)
{
   unsigned char data[MAX_IP_PACKET_SIZE];
   struct Command *cmd = NULL;
   struct CommandCbArg *cmds = (struct CommandCbArg*)arg;
   size_t dataLen;
   struct sockaddr_in src;
   data[0] = 0;

   // should only be read events, but make sure
   if (type == EVENT_FD_READ) {
      // read from the socket to get the command and it's data
      dataLen = socket_read(socket, data, MAX_IP_PACKET_SIZE, &src);

      // make sure something was actually read
      if (dataLen > 0) {
         cmd = cmds->cmds + *data;
         DBG_print(DBG_LEVEL_INFO, "Received command 0x%02x (%d - %d - %d)",
                                       *data, cmd->prot, cmd->uid, cmd->group);

         // Check to see if command is protected
         if (cmd->prot == CMD_PROTECTED) {
            //NOTE(Joshua Anderson): Cryptography support was reomved for now, so this is now a No-OP.
            DBG_print(DBG_LEVEL_INFO, "Protected command, attempting to validate\n");
         } else {
            // Un-protected command, nothing out of the ordinary here
            (*(cmd->cmd_cb))(socket, *data, data+1, dataLen-1, &src);
         }
      }
   }

   return EVENT_KEEP;
}

int tx_cmd_handler_cb(int socket, char type, void * arg)
{
   unsigned char data[MAX_IP_PACKET_SIZE];
   // struct ProcessData *proc = (struct ProcessData*)arg;
   size_t dataLen;
   struct sockaddr_in src;
   data[0] = 0;

   // should only be read events, but make sure
   if (type == EVENT_FD_READ) {
      // read from the socket to get the command and it's data
      dataLen = socket_read(socket, data, MAX_IP_PACKET_SIZE, &src);

      // make sure something was actually read
      if (dataLen > 0) {
         DBG_print(DBG_LEVEL_INFO, "Received TX command response 0x%02x", data[0]);
      }
   }

   return EVENT_KEEP;
}

void invalidCommand(int socket, unsigned char cmd, void * data, size_t dataLen, struct sockaddr_in * src)
{
   // tell the source of the command they messed up
   //socket_write(socket, "Invalid command", strlen("Invalid command"), src);
   DBG_print(DBG_LEVEL_INFO, "Received invalid command: 0x%02x\n", cmd);
}

void cmd_handler_cleanup(struct CommandCbArg *cmds)
{
   if (cmds && cmds->cmds) {
      free(cmds->cmds);
   }
}
