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
 * @file: cmd.h Command Handler Header File
 *
 * This file contains the prototype for command functions
 * in addition to global commands that all processes utilize.
 *
 * @author Greg Manyak
 */

#ifndef CMD_H
#define CMD_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "ipc.h"

/**
 * There are a few global command bytes
 * that all processes need to be aware of.
 */

/// Command byte for registering static processes with the s/w watchdog
#define CMD_WDT_REGISTER      0x03
/// Command byte for registering temporary events with s/w watchdog
#define CMD_WDT_TEMP_REG      0x04
/// Command byte for canceling temporary watch with s/w watchdog
#define CMD_WDT_TEMP_CANCEL   0x05
/// Command byte for validating configuration
#define CMD_WDT_VALIDATE      0x06

/// Command byte sent to watch requester after a temporary watch timeout
#define CMD_WDT_TEMP_TIMEOUT  0xAA

/// Command byte for requesting status struct
#define CMD_STATUS_REQUEST    0x01

/// Command byte for responding to a status query
#define CMD_STATUS_RESPONSE   0xF1

/// Maximum number of commands
#define MAX_NUM_CMDS 256

struct EventState;
struct IPC_Command;
struct ProcessData;
struct CommandCbArg;
struct XDR_StructDefinition;

// Format for a command callback function
typedef void (*CMD_handler_t)(int socket, unsigned char cmd, void *data,
   size_t dataLen, struct sockaddr_in *fromAddr);

// Format for a XDR command callback function
typedef void (*CMD_XDR_handler_t)(struct ProcessData *, struct IPC_Command *cmd,
   struct sockaddr_in *fromAddr, void *arg, int fd);

// Format for a multicast command callback function
typedef void (*MCAST_handler_t)(void *arg, int socket, unsigned char cmd,
   void *data, size_t dataLen, struct sockaddr_in *fromAddr);

// Initializes the command callbacks
int cmd_handler_init(const char * process_name, struct ProcessData *proc,
      struct CommandCbArg **cmds);

// The actual command handler callback for the event handler
int cmd_handler_cb(int socket, char type, void *arg);

void cmd_handler_cleanup(struct CommandCbArg **cmds);

void cmd_set_multicast_handler(struct CommandCbArg *st,
   struct EventState *evt_loop, const char *service, int cmdNum,
   MCAST_handler_t handler, void *arg);

void cmd_remove_multicast_handler(struct CommandCbArg *st,
   const char *service, int cmdNum, struct EventState *evt_loop);

void cmd_cleanup_cb_state(struct CommandCbArg *st, struct EventState *evt_loop);

int tx_cmd_handler_cb(int socket, char type, void * arg);

typedef void (*CMD_struct_itr)(uint32_t type, struct XDR_StructDefinition *,
      char *buff, size_t len, void *arg1, int arg2);
extern int CMD_iterate_structs(char *src, size_t len, CMD_struct_itr itr_cb,
      void *arg1, int arg2);

// Functions to send and receive commands
struct CMD_MulticallInfo {
   int (*func)(struct CMD_MulticallInfo *mc, const char *commandName, 
         int argc, char **argv, const char *host);
   const char *name;
   const char *opt;
   const char *help_description;
   const char *help_param_summary;
   const char *help_detail;
};

struct CMD_XDRCommandInfo {
   uint32_t command;
   uint32_t params;
   const char *name;
   const char *summary;
   uint32_t *types;
   struct XDR_StructDefinition *parameter;
   CMD_XDR_handler_t handler;
   void *arg;
};

#ifdef __cplusplus
enum XDR_PRINT_STYLE : short;
#else
enum XDR_PRINT_STYLE;
#endif

extern void CMD_register_commands(struct CMD_XDRCommandInfo*, int);
extern void CMD_register_command(struct CMD_XDRCommandInfo*, int);
extern void CMD_set_xdr_cmd_handler(uint32_t num, CMD_XDR_handler_t cb,
      void *arg);
extern int CMD_xdr_cmd_help(struct CMD_XDRCommandInfo *command);
extern int CMD_mc_cmd_help(struct CMD_MulticallInfo *command);
extern int CMD_send_command_line_command(int argc, char **argv,
      struct CMD_MulticallInfo *mc, struct ProcessData *proc,
      IPC_command_callback cb, void *cb_arg, unsigned int timeout,
      const char *destProc, enum XDR_PRINT_STYLE *styleOut);
int CMD_send_command_line_command_sync(int argc, char **argv,
      struct CMD_MulticallInfo *mc, IPC_command_callback cb,
      void *cb_arg, unsigned int timeout, const char *destProc,
      enum XDR_PRINT_STYLE *styleOut);
extern int CMD_resolve_callback(struct ProcessData *proc, IPC_command_callback cb,
      void *arg, enum IPC_CB_TYPE cb_type, void *rxbuff, size_t rxlen);

extern void CMD_print_response(struct ProcessData *proc, int timeout,
      void *arg, char *resp_buff, size_t resp_len, enum IPC_CB_TYPE cb_type);

struct CMD_ErrorInfo {
   uint32_t id;
   const char *name;
   const char *description;
};

extern void CMD_register_error(struct CMD_ErrorInfo *errs);
extern void CMD_register_errors(struct CMD_ErrorInfo *errs);
extern const char *CMD_error_message(uint32_t id);
extern struct IPC_OpaqueStruct CMD_struct_to_opaque_struct(void *src, uint32_t type);
extern void CMD_add_response_cb(struct ProcessData *proc, uint32_t id,
      struct sockaddr_in host,
      IPC_command_callback cb, void *arg,
      enum IPC_CB_TYPE cb_type, unsigned int timeout);

extern int CMD_set_cmd_handler(struct CommandCbArg *cmd,
            int cmdNum, CMD_handler_t handler, uint32_t uid,
            uint32_t group, uint32_t protection);
extern int CMD_loopback_cmd(struct CommandCbArg *cmds, int fd,
            int cmdNum, void *data, size_t dataLen);

#ifdef __cplusplus
}
#endif

#endif

