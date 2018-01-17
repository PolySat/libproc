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

#ifdef __cplusplus
extern "C" {
#endif

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

// Format for a command callback function
typedef void (*CMD_handler_t)(int socket, unsigned char cmd, void *data,
   size_t dataLen, struct sockaddr_in *fromAddr);

// Format for a multicast command callback function
typedef void (*MCAST_handler_t)(void *arg, int socket, unsigned char cmd,
   void *data, size_t dataLen, struct sockaddr_in *fromAddr);

struct Command {
   CMD_handler_t cmd_cb;
   uint32_t uid, group, prot;
};

struct MulticastCommandState;
struct ProcessData;

struct CommandCbArg {
   struct Command *cmds;
   struct McastCommandState *mcast;
   struct ProcessData *proc;
};

// Initializes the command callbacks
int cmd_handler_init(const char * process_name, struct CommandCbArg *cmds);

// The actual command handler callback for the event handler
int cmd_handler_cb(int socket, char type, void *arg);

void cmd_handler_cleanup(struct CommandCbArg *cmds);

void cmd_set_multicast_handler(struct CommandCbArg *st,
   struct EventState *evt_loop, const char *service, int cmdNum,
   MCAST_handler_t handler, void *arg);

void cmd_remove_multicast_handler(struct CommandCbArg *st,
   const char *service, int cmdNum, struct EventState *evt_loop);

void cmd_cleanup_cb_state(struct CommandCbArg *st, struct EventState *evt_loop);

int tx_cmd_handler_cb(int socket, char type, void * arg);

#ifdef __cplusplus
}
#endif

#endif

