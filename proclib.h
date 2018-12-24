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
 * Process Library
 *
 * @author Greg Eddington
 */
#ifndef _PROC_LIB_H
#define _PROC_LIB_H

#include "events.h"
#include "ipc.h"
#include "cmd.h"
#include "critical.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Path where all of the .pid files are stored
#define PID_FILE_PATH "/var/run"
/// Path where all of the .proc files are stored
#define PROC_FILE_PATH "/var/run"

#define SCHEDULE_KEY_MAX 128
#define NOT_SCHEDULED -1


enum WatchdogMode {
   /// Constant for enabling the watchdog when initializing a process.
   WD_ENABLED = 1,
   /// Constant for disabling the watchdog when initializing a process.
   WD_DISABLED = 0,
};

/** Type which contains event handler information **/
typedef struct ProcessData {
   EVTHandler *evtHandler;
   int keyToIdMap[SCHEDULE_KEY_MAX];
   //Socket
   int cmdFd, txFd;
   int sigPipe[2];
   struct ProcSignalCB *signalCBHead;
   struct ProcChild *childHead;
   struct ProcWriteNode *writeListHead;
   char *name;
   int cmdPort;
   void *callbackContext;
   //cmds holds the parsed, .cmd.cfg file call backs along with other info
   struct CommandCbArg cmds;
   struct CSState criticalState;
} ProcessData;

/** Returns the EVTHandler context for the process.  Needed to directly call
  * EVT_* functions.
  **/
EVTHandler *PROC_evt(ProcessData *proc);

struct XDR_CommandHandlers {
   uint32_t number;
   CMD_XDR_handler_t cb;
   void *arg;
};
/**
 * Initializes the process and creates an event handler which can
 * be used by the process to register callbacks.
 *
 * The watchdog mode controls watchdog behavior:
 * - If WD_ENABLED is used, libproc will register the process with the software process
 * - If WD_DISABLED is used, libproc will skip process watchdog initialization.
 *
 * @param procName The unique process name.
 * @param wdMode Software watchdog mode.
 * @param handlers An array, terminated with an all-zero entry, of XDR
 *                  command handlers to register with the CMD system
 *
 * @return A ProcessData struct which can be used to register callbacks.
 *
 * @retval Initialzed ProcessData struct on success
 * @retval Null		Error
 */
ProcessData *PROC_init_xdr(const char *procName, enum WatchdogMode wdMode,
      struct XDR_CommandHandlers *handlers);
ProcessData *PROC_init(const char *procName, enum WatchdogMode wdMode);

/**
 * Registers the process with the software watchdog.
 * Note: If the process was initialized with WD_ENABLE, this step is redundant.
 *
 * @param proc The process object.
 *
 * @retval None
 */
void PROC_wd_enable(ProcessData *proc);

/**
 * Sets the context pointer passed by the process library into some of
 * the callbacks.  This is a separate function to preserve
 * API compatibility.
 *
 * @param proc The process object.
 * @param ctx The context object to pass into subsequent callbacks.
 *
 * @retval None
 */
void PROC_set_context(ProcessData *proc, void *ctx);

/**
 * Sends an CMD message over the process' primary IPC socket to the named
 *    service.
 *
 * @param proc    The process object.
 * @param cmd     The command to be sent.
 * @param data    Data and/or arguments corresponding to command.
 * @param dataLen Length of the data, in bytes.
 * @param dest    Name of the destination service.
 *
 */

int PROC_cmd(ProcessData *proc, unsigned char cmd, void *data, size_t dataLen, const char *dest);
int PROC_cmd_raw_sockaddr(ProcessData *proc, void *data, size_t dataLen,
      struct sockaddr_in *dest);

/**
 * Sends an CMD message over the process' secondary IPC socket to the
 *  named service.  All responses get ignored.  The secondary socket
 *  is useful so responses don't get intermingled with commands from other
 *  processes.
 *
 * @param proc    The process object.
 * @param cmd     The command to be sent.
 * @param data    Data and/or arguments corresponding to command.
 * @param dataLen Length of the data, in bytes.
 * @param dest    Name of the destination service.
 *
 */
int PROC_cmd_secondary(ProcessData *proc, unsigned char cmd, void *data,
   size_t dataLen, const char *dest);

/**
 * Sends a Multicast CMD message over the process' primary IPC socket to all
 *  listening processes.
 *
 * @param proc    The process object.
 * @param cmd     The command to be sent.
 * @param data    Data and/or arguments corresponding to command.
 * @param dataLen Length of the data, in bytes.
 *
 */
int PROC_multi_cmd(ProcessData *proc, unsigned char cmd, void *data,
   size_t dataLen);

/**
 * Sends an CMD message to yourself
 *
 * @param proc    The process object.
 * @param cmd     The command to be sent.
 * @param data    Data and/or arguments corresponding to command.
 * @param dataLen Length of the data, in bytes.
 *
 */
int PROC_loopback_cmd(struct ProcessData *proc,
            int cmdNum, void *data, size_t dataLen);

/**
 * Sends an IPC cmd message over the process' primary IPC socket to the
 *    desired socket.
 *
 * Most likely use is as reply to a command.
 *
 * @param proc    The process object.
 * @param cmd     The command to be sent.
 * @param data    Data and/or arguments corresponding to command.
 * @param dataLen Length of the data, in bytes.
 * @param dest    Destination socket address structure.
 *
 */
int PROC_cmd_sockaddr(ProcessData *proc, unsigned char cmd, void *data, size_t dataLen, struct sockaddr_in *dest);

/**
 * Sends an IPC cmd message over the process' secondary IPC socket to the
 *    desired socket.  The secondary socket is useful so command responses
 *    don't get mixed in with commands from other processes.
 *
 * Most likely use is as reply to a command.
 *
 * @param proc    The process object.
 * @param cmd     The command to be sent.
 * @param data    Data and/or arguments corresponding to command.
 * @param dataLen Length of the data, in bytes.
 * @param dest    Destination socket address structure.
 *
 */
int PROC_cmd_sockaddr_secondary(ProcessData *proc, unsigned char cmd, void *data, size_t dataLen, struct sockaddr_in *dest);

/**
 * Cleans the process up.
 *
 * @param proc
 */
void PROC_cleanup(ProcessData *proc);

/** Signal callback **/
typedef int (*PROC_signal_cb)(int sig, void *p);

/** Register a signal handler
 * @param ctx The event state
 * @param sigNum The signal number
 * @param cb The signal callback
 * @param p The parameters
 */
int PROC_signal(struct ProcessData *proc, int sigNum, PROC_signal_cb cb,
      void *p);

/** Replace the command handler for the given command with a new one.
 * @param proc The process state
 * @param cmdNum The command number to replace
 * @param handler The function pointer of the new command handler
 * @param uid The command's protection UID
 * @param group The command's protection group
 * @param protection Boolean value indicating if this is a protected command
 */
int PROC_set_cmd_handler(struct ProcessData *proc,
      int cmdNum, CMD_handler_t handler, uint32_t uid, uint32_t group, uint32_t protection);

/** Replace the command handler for the given multicast command with a new one.
 * @param proc The process state
 * @param service The name of the service to receive multicast packets from
 * @param port The port number the command is sent over.  Pass 0 to use
 *              the service's default port.
 * @param cmdNum The command number to replace.  Use a negative value to
 *                   register for all commands.
 * @param handler The function pointer of the new command handler
 * @param arg The opaque value to pass back into the callback
 */
int PROC_set_multicast_handler(struct ProcessData *proc, const char *service,
      int cmdNum, MCAST_handler_t handler, void *arg);

/**
 * Returns the process' assigned UDP port id
 *
 * @param proc    The process object.
 *
 */
int PROC_udp_id(ProcessData *proc);


#define FREE_DATA_AFTER_WRITE 1
#define COPY_DATA_TO_WRITE 2
#define IGNORE_DATA_AFTER_WRITE 3

/** Perform a nonblocking write on a fd.  This method queues the data
 *  for transmission when a write won't block and returns immediately
 * @param proc The process state
 * @param fd The file descriptor to write the data to
 * @param data A pointer to the data to write
 * @param memoryOptions One of three memory options indicating how to treat the data
 */
int PROC_nonblocking_write(struct ProcessData *proc, int fd, uint8_t *data, uint32_t len, int memoryOptions);

typedef void (*proc_nb_write_cb)(int fd, uint8_t *data, uint32_t len, void *arg);
/** Perform a nonblocking write on a fd.  This method queues the data
 *  for transmission when a write won't block and returns immediately
 * @param proc The process state
 * @param fd The file descriptor to write the data to
 * @param data A pointer to the data to write
 * @param cb The callback to be called with the descriptor fd indicates it can be written to.
 * @param arg The opaque parameter passed into the callback cb
 * @param memoryOptions One of three memory options indicating how to treat the data
 */
int PROC_nonblocking_write_callback(struct ProcessData *proc, int fd, uint8_t *data, uint32_t len, proc_nb_write_cb cb, void *arg, int memoryOptions);

/** Reads a process' critical state from critical state storage.  This
 *    storage is checksumed and redundant for higher reliability.  It can
 *    hold no more than 224 bytes of state data.
 * @param proc The process state
 * @param state A pointer to the location in memory the state should be loaded
 * @param len The number of bytes to load (length), not to exceed 224
 * @returns The number of bytes read, or a negative error code
 */
int PROC_read_critical_state(ProcessData *proc, void *state, int len);

/** Saves a process' critical state from critical state storage.  This
 *    storage is checksumed and redundant for higher reliability.  It can
 *    hold no more than 224 bytes of state data.
 * @param proc The process state
 * @param state A pointer to the location in memory containing the state to
 *              write to storage.
 * @param len The number of bytes to save (length), not to exceed 224
 * @returns The number of bytes written, or a negative error code
 */
int PROC_save_critical_state(ProcessData *proc, void *state, int len);

#define CHILD_STATE_INIT 1
#define CHILD_STATE_RUNNING 2
#define CHILD_STATE_FLUSH_PIPES 3
#define CHILD_STATE_DONE 4
#define CHILD_BUFF_ERR -2
#define CHILD_BUFF_STEAL_BUFF -3
#define CHILD_BUFF_NORM 0
#define CHILD_BUFF_CLOSING 1
#define CHILD_BUFF_NOROOM 2

struct ProcChild;

typedef void (*CHLD_death_cb_t)(struct ProcChild *child, void *arg);

typedef int (*CHLD_buf_stream_cb_t)(struct ProcChild *child, int lastchance,
      void *arg, char *buff, int len);

/** Structure to hold information about proclib created child processes **/
typedef struct BufferedStreamState {
   CHLD_buf_stream_cb_t cb;
   void *arg;
   char *buff;
   int buffLen, buffCap;
} BufferedStreamState;

typedef struct ProcChild {
   struct ProcessData *parentData;
   pid_t procId;
   struct rusage rusage;
   int exitStatus, state;
   int stdin_fd, stdout_fd, stderr_fd;
   BufferedStreamState streamState[2];

   CHLD_death_cb_t deathCb;
   void *deathArg;

   struct ProcChild *next;
} ProcChild;

/**
  * Fork a new process running the given command.  The returned ProcChild
  *  structure enables to caller to access the stdin, stdout, and stderr
  *  streams of the child.  Zombies are automatically reaped.  Use this
  *  function for all process creation.
  *
  * @param proc The process data structure for the running process
  * @param cmdFmt A format string, using the same syntax as printf, that
  *   describes the executable name and all command line parameters.
  * @param ... The additional printf-style parameters for cmdFmt.
  * @returns A ProcChild structure pointer, or NULL if an error occurse.
  */
ProcChild *PROC_fork_child(struct ProcessData *proc, const char *cmdFmt, ...);

/**
  * Wrapper function for PROC_fork_child. Allows redirection of
  * input/output. Accepts a format string.
  *
  * @param proc The process data structure for the running process
  * @param inFd_read Change the read end of stdin.
  * @param inFd_write Change the write end of stdin.
  * @param outFd_read Change the read end of stdout.
  * @param outFd_out Change the write end of stdout.
  * @param errFd_read Change the read end of stderr.
  * @param errFd_write Change the write of stderr.
  * @param cmdFmt A format string, using the same syntax as printf, that
  *   describes the executable name and all command line parameters.
  *
  * @returns A ProcChild structure pointer, or NULL if an error occurse.
  */
ProcChild *PROC_fork_child_va(struct ProcessData *proc, int inFd_read, int inFd_write,
      int outFd_read, int outFd_write, int errFd_read, int errFd_write, struct rlimit *cpu_limit,
      struct rlimit *mem_limit, const char *cmdFmt, ...);

/**
  * Wrapper function for PROC_fork_child. Allows redirection of
  * input/output. Accepts a va_list.
  *
  * @param proc The process data structure for the running process
  * @param inFd_read Change the read end of stdin.
  * @param inFd_write Change the write end of stdin.
  * @param outFd_read Change the read end of stdout.
  * @param outFd_out Change the write end of stdout.
  * @param errFd_read Change the read end of stderr.
  * @param errFd_write Change the write of stderr.
  * @param ap A variable argument list.
  *
  * @returns A ProcChild structure pointer, or NULL if an error occurse.
  */
ProcChild *PROC_fork_child_fd(struct ProcessData *proc, int inFd_read, int inFd_write,
      int outFd_read, int outFd_write, int errFd_read, int errFd_write, struct rlimit *cpu_limit,
      struct rlimit *mem_limit, const char *cmdFmt, va_list ap);


/** Closes a child's file descriptor.  May cause the termination notification
  *  function to be called if the zombie has already been reaped.
  **/
void CHLD_close_fd(ProcChild *child, int fd);

/** Closes a child's stdin file descriptor.  Call this immediately when there
  * is no more data to be written to stdin.  May cause the termination
  * notification function to be called if the zombie has already been reaped.
  **/
void CHLD_close_stdin(ProcChild *child);

/** Ignores all data a child writes to stderr.  Use this instead of closing
  *  the stderr file descriptor to avoid causing a SIGPIPE in the child.
  **/
char CHLD_ignore_stderr(ProcChild *child);

/** Ignores all data a child writes to stdout.  Use this instead of closing
  *  the stdout file descriptor to avoid causing a SIGPIPE in the child.
  **/
char CHLD_ignore_stdout(ProcChild *child);

/** Registers a death callback function for a child.  The function is called
  *  once, after the OS has notified the process of the child's death AND
  *  all data remaining on input / output streams have been processed.
  *  The resource usage values and exit status are valid when this function
  *  is called.  Only one death function can be registered.
  *
  * @param child THe child to watch for death
  * @param deathCb The function to call when the death occurs
  * @param arg An opaque pointer to pass directly to the death function.
  **/
char CHLD_death_notice(ProcChild *child, CHLD_death_cb_t deathCb, void *arg);

char CHLD_stdout_reader(ProcChild *child, CHLD_buf_stream_cb_t, void *arg);
char CHLD_stderr_reader(ProcChild *child, CHLD_buf_stream_cb_t, void *arg);


/*
* Runs the specified function in a different thread
* @param proc The process data pointer
* @param fcn_ptr Pointer to the function that will be run
* @param arg Opaque argument that will be passed to the function
*/
int thread_function(ProcessData *proc, void *fcn_ptr, void *arg, void *cb_fcn,
void *cb_arg);

#ifdef __cplusplus
}

class Process
{
   public:
      Process(const char *name, enum WatchdogMode wdMode) : events(NULL)
         { process = PROC_init(name, wdMode); }
      virtual ~Process() {
         delete events;
         if (process)
            PROC_cleanup(process);
      }
      EventManager *event_manager() {
         if (!events)
            events = new EventManager(PROC_evt(process));
         return events;
      }

      int AddSignalEvent(int sigNum, PROC_signal_cb cb, void *p)
         { return PROC_signal(process, sigNum, cb, p); }

      ProcessData *getProcessData(void) { return process; }

   private:
      Process() { process = NULL; }
      ProcessData *process;
      EventManager *events;
};

#endif

#endif
