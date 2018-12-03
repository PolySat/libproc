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
 * @author Dr. John Bellardo
 * @author Greg Manyak
 */
#include "proclib.h"
#include "events.h"
#include "cmd.h"
#include "debug.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <assert.h>
#include "watchdog_cmd.h"
#include <time.h>
#include "critical.h"
#include <pthread.h>
#include "ipc.h"

#define READ_BUFF_MIN 4096
#define READ_BUFF_MAX (READ_BUFF_MIN * 4)
#define WATCHDOG_VALIDATE_SECS 30

static int signalWriteFD = -1;

static int sigchld_handler(int, void*);
static int setup_signal_fd(ProcessData *proc);

//When a socket is written to, this is the call back that is called
static int socket_write_cb(int fd, char type, void * arg);

/* Structure which defines a signal callback */
struct ProcSignalCB
{
   PROC_signal_cb cb;      /* A pointer to the signal callback function */
   void *arg;                /* The arguments to pass */
   int sigNum;             /* The signal number to respond to */
   int recvdCnt;
   struct ProcSignalCB *next;  /* The next signal callback */
};

#define FREE_DATA_AFTER_WRITE 1
#define COPY_DATA_TO_WRITE 2
#define IGNORE_DATA_AFTER_WRITE 3

/* Linked list to hold writes pending notification that the write will not
 * block
 */
struct ProcWriteNode {
   uint8_t *data;
   uint32_t len;
   int fd;
   int freeMem;
   proc_nb_write_cb cb;
   void *arg;

   struct ProcWriteNode *next;
};


/** Returns the EVTHandler context for the process.  Needed to directly call
  * EVT_* functions.
  **/
EVTHandler *PROC_evt(ProcessData *proc)
{
   if (!proc)
      return NULL;
   return proc->evtHandler;
}

struct CSState *proc_get_cs_state(ProcessData *proc)
{
   if (!proc)
      return NULL;
   return &proc->criticalState;
}

static ProcessData *watchProc = NULL;
static int proc_cmd_sockaddr_internal(ProcessData *proc, int fd, unsigned char cmd, void *data, size_t dataLen, struct sockaddr_in *dest);

static void watchdog_reg_info(int fd, unsigned char cmd, void *data,
   size_t dataLen, struct sockaddr_in *src)
{
   WatchdogInfoResp *resp = (WatchdogInfoResp*)data;

   if (!watchProc || !watchProc->name)
      return;

   // If our entry looks good, no further action is needed
   if (dataLen >= strlen(watchProc->name) + sizeof(*resp) &&
            0 == strcmp(watchProc->name, resp->name) &&
            getpid() == ntohs(resp->pid) &&
            resp->flags & WDG_PROC_FLAG_WATCHED)
      return;

   // Re-register!
   PROC_cmd(watchProc, CMD_WDT_REGISTER, (unsigned char*)watchProc->name,
      strlen(watchProc->name)+1, "watchdog");
}

static int validate_watch_registration(void *arg)
{
   ProcessData *proc = (ProcessData*)arg;

   if (!proc || !proc->name)
      return EVENT_KEEP;

   watchProc = proc;
   PROC_cmd(proc, WATCHDOG_CMD_REG_INFO, (unsigned char*)proc->name,
         strlen(proc->name)+1, "watchdog");

   return EVENT_KEEP;
}

void PROC_set_context(ProcessData *proc, void *ctx)
{
   proc->callbackContext = ctx;
}

int PROC_udp_id(ProcessData *proc)
{
   if (!proc)
      return 0;

   return proc->cmdPort;
}

static void PROC_debugger_state(struct IPCBuffer *buff, void *arg)
{
   ProcessData *proc = (ProcessData*)arg;

   ipc_printf_buffer(buff, "  \"process_name\": \"%s\",\n", proc->name);
}

//Initializes a ProcessData object
ProcessData *PROC_init(const char *procName, enum WatchdogMode wdMode)
{
   ProcessData *proc;
   char filepath[80];
   char oldname[80];
   int fd;
   FILE *inp;
   int oldPid = -1;

#ifdef TIME_TEST
   struct timeval startTime;
   struct timeval endTime;
   struct timeval totalTime;

   gettimeofday(&startTime, NULL);
#endif //TIME_TEST
   // Allocate our internal state structure
   proc = (ProcessData*)malloc(sizeof(ProcessData));
   if (!proc)
      return NULL;
   memset(proc, 0, sizeof(*proc));

   // Allocate enough space for process name and terminating null byte
   if (procName) {
      proc->name = (char*)malloc(strlen(procName)+1);
      strcpy(proc->name, procName);
   }
   else
      proc->name = NULL;

   // Configure the debug interface
   DBG_init(proc->name);
   DBG_setLevel(DBG_LEVEL_WARN);

   // write .proc file (file pid.proc contains process name)
   if (proc->name) {
      proc->cmdPort = socket_get_addr_by_name(proc->name);

      // Read in old .pid file, if it exists
      sprintf(filepath, "%s/%s.pid", PID_FILE_PATH, procName);
      inp = fopen(filepath, "r");
      if (inp) {
         if (1 != fscanf(inp, "%d", &oldPid))
            oldPid = -1;
         fclose(inp);
      }

      // Delete old .proc file if it claims to be our process
      if (oldPid > 1) {
         sprintf(filepath, "%s/%d.proc", PROC_FILE_PATH, oldPid);
         inp = fopen(filepath, "r");
         if (inp) {
            oldname[0] = 0;
            if(fscanf(inp, "%s", oldname) == 1) {
               fclose(inp);
               if (0 == strcmp(oldname, procName))
                  unlink(filepath);
            }
         }
      }

      sprintf(filepath, "%s/%d.proc", PROC_FILE_PATH, getpid());
      fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 644);

      if (fd != -1) {
         int len = strlen(procName);
         if(write(fd, procName, len) < len) {
               ERRNO_WARN("Failed to write proc file\n");
         }
         ERR_WARN(close(fd), "Failed to close proc file\n");
      } else {
         DBG_print(DBG_LEVEL_WARN,"Failed to open proc file, %s\n", filepath);
      }

      // Clear variables before using them again
      memset(filepath, 0, sizeof(filepath));

      // write .pid file (file processName.pid that contains the process id)
      sprintf(filepath, "%s/%s.pid", PID_FILE_PATH, procName);
      fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 644);

      if (fd != -1) {
         // Re-purpose filepath variable here for pid
         memset(filepath, 0, sizeof(filepath));
         sprintf(filepath, "%d", getpid());

         int len = strlen(filepath);
         if(write(fd, filepath, strlen(filepath)) < len) {
               ERRNO_WARN("Failed to write pid file\n");
         }

         ERR_WARN(close(fd), "Failed to close pid file\n");
      } else {
         DBG_print(DBG_LEVEL_WARN,"Failed to open pid file, %s\n", filepath);
      }
   }

   ERR_EXIT(proc->cmdFd = socket_named_init(procName),                 //Exit?
         "Failed to open socket for %s\n", procName);

   ERR_EXIT(proc->txFd = socket_init(0),                 //Exit?
         "Failed to open TX socket for %s\n", procName);

   // Initialize event handler and return it
   proc->evtHandler = EVT_create_handler(PROC_debugger_state, proc);
   EVT_set_debugger_port(proc->evtHandler, socket_get_addr_by_name(proc->name));
   //set up sigPipe to take signals. Signals will be treated as an event with cb 'signal_fd_cb'
   setup_signal_fd(proc);

   // initialize command handler: commands are loaded from .cmd.cfg file
   if (cmd_handler_init(procName, &proc->cmds) == -1) {
      return NULL;
   }
   //Event for when something (probably a command) appears on the fd
   EVT_fd_add(proc->evtHandler, proc->cmdFd, EVENT_FD_READ, cmd_handler_cb, &proc->cmds);
   //Event for when something (probably a command response) appears on the fd
   EVT_fd_add(proc->evtHandler, proc->txFd, EVENT_FD_READ, tx_cmd_handler_cb, proc);
   //Set up SIGCHLD signal handler
   PROC_signal(proc, SIGCHLD, &sigchld_handler, proc);

   // Register process with the s/w watchdog (make sure to send null byte!)
   if (procName && wdMode == WD_ENABLED) {
      PROC_wd_enable(proc);
   }

   critical_state_init(&proc->criticalState, proc->name);
#if TIME_TEST
   gettimeofday(&endTime, NULL);
   timersub(&endTime, &startTime, &totalTime);
   DBG_print(DBG_LEVEL_WARN, "Process initialization time: %d.%06d\n", totalTime.tv_sec, totalTime.tv_usec);
#endif //TIME_TEST

   return proc;
}

void PROC_wd_enable(ProcessData *proc) {
   PROC_set_cmd_handler(proc, WATCHDOG_CMD_REG_INFO_RESP, &watchdog_reg_info,
      0, 0, 0);
   EVT_sched_add_with_timestep(PROC_evt(proc), EVT_ms2tv(0),
      EVT_ms2tv(WATCHDOG_VALIDATE_SECS * 1000),
      &validate_watch_registration, proc);
}

//Cleans the process up.
void PROC_cleanup(ProcessData *proc)
{
   char filepath[80];

   if (!proc) //Already clean
      return;

   cmd_cleanup_cb_state(&proc->cmds, proc->evtHandler);
   critical_state_cleanup(&proc->criticalState);

   // Clear errno to prevent false errors
   errno = 0;
   EVT_free_handler(proc->evtHandler);
   close(proc->sigPipe[0]);
   ERRNO_WARN("close sigPipe[0] error: ");
   close(proc->sigPipe[1]);
   ERRNO_WARN("close sigPipe[1] error: ");
   close(proc->cmdFd);
   ERRNO_WARN("close cmdFd error: ");
   close(proc->txFd);
   ERRNO_WARN("close txFd error: ");
   signalWriteFD = -1;

   if (proc->name) {
      //** Remove the .pid and .proc files **
      // write .proc file (file pid.proc contains process name)
      sprintf(filepath, "%s/%d.proc", PROC_FILE_PATH, getpid());
      unlink(filepath);
      ERRNO_WARN("unlink error: ");

      // Clear path before using it again
      memset(filepath, 0, sizeof(filepath));

      // write .pid file (file processName.pid that contains the process id)
      sprintf(filepath, "%s/%s.pid", PID_FILE_PATH, proc->name);
      unlink(filepath);
      ERRNO_WARN("unlink error: ");

      free(proc->name);
   }

   struct ProcSignalCB *curr;

   while((curr = proc->signalCBHead)) {
      proc->signalCBHead = curr->next;
      free(curr);
   }

   cmd_handler_cleanup(&proc->cmds);

   free(proc);
}

static void validate_pipe_flush(ProcChild *child)
{
   struct ProcChild **curr;
   //Child is never null when passed in
   if (child->state != CHILD_STATE_FLUSH_PIPES)
      return;

   if (child->stdin_fd != -1 || child->stdout_fd != -1 ||
         child->stderr_fd != -1)
      return;

   child->state = CHILD_STATE_DONE;

   if (child->deathCb)
      (*child->deathCb)(child, child->deathArg);

   // Remove node from linked list
   for (curr = &child->parentData->childHead ; *curr; curr = &(*curr)->next) {
      if ( (*curr) == child) {
         // Found the parent; unlink and free the child
         *curr = child->next;
         child->next = NULL;
         if (child->streamState[0].buff)
            free(child->streamState[0].buff);
         if (child->streamState[1].buff)
            free(child->streamState[1].buff);
         free(child);
         break;
      }
   }
}

static int sigchld_handler(int signum, void *param)
{
   ProcessData *proc = (ProcessData*)param;
   int more;
   pid_t cpid;
   struct rusage rusage;
   int exitStatus;
   struct ProcChild *curr, *tmp;

   do {
      cpid = wait4(-1, &exitStatus, WNOHANG, &rusage);
      more = 0 < cpid;
      if (cpid < 0) {
         if (ECHILD != errno)
            ERRNO_WARN("Error with wait4");
      }
      else if (cpid > 0) {
         for(curr = proc->childHead; curr; curr = tmp) {
            // Use tmp for iteration in case child is deallocated
            tmp = curr->next;
            if (curr->procId != cpid || curr->state == CHILD_STATE_DONE)
               continue;

            curr->rusage = rusage;
            curr->exitStatus = exitStatus;
            curr->state = CHILD_STATE_FLUSH_PIPES;
            validate_pipe_flush(curr);
         }
      }
   } while(more);
   return EVENT_KEEP;
}

int signal_fd_cb(int fd, char type, void *arg)
{
   ProcessData *proc = (ProcessData*)arg;
   int rd, signum;
   static char rdBuff[sizeof(signum)];
   static int rdLen = 0;
   struct ProcSignalCB **curr, *sigTmp;
   int keep;

   do {
      rd = read(proc->sigPipe[0], &rdBuff[rdLen], sizeof(rdBuff) - rdLen);
      // more = rd > 0 || (-1 == rd && EAGAIN == errno);
      if (-1 >= rd && EAGAIN != errno) {
         ERRNO_WARN("signal fd error, closing:");
         return EVENT_REMOVE;
      }
      if (0 == rd) {
         DBG_print(DBG_LEVEL_WARN, "signal fd closed, removing event");
         return EVENT_REMOVE;
      }

      rdLen += rd;
      if (rdLen == sizeof(rdBuff)) {
         rdLen = 0;
         memcpy(&signum, rdBuff, sizeof(rdBuff));

         for (curr = &proc->signalCBHead; *curr; ) {
            sigTmp = *curr;

            if (sigTmp->sigNum == signum) {
               sigTmp->recvdCnt++;
               //cb is assigned in PROC_signal
               keep = (*sigTmp->cb)(signum, sigTmp->arg);

               if (EVENT_REMOVE == keep) {
                  *curr = sigTmp->next;
                  free(sigTmp);
               }
               else
                  curr = &sigTmp->next;
            }
            else
               curr = &sigTmp->next;
         }
      }
   } while(0);

   return EVENT_KEEP;
}

static void PROC_signal_handler(int sigNum, siginfo_t *si, void *p)
{
   if (signalWriteFD != -1)
      if(write(signalWriteFD, &sigNum, sizeof(sigNum)) < sizeof(sigNum) )
      	ERRNO_WARN("Write error");
}

static int setup_signal_fd(ProcessData *proc)
{
   int currFlags;
   if (-1 != signalWriteFD)
      return 0;

   if (0 != pipe(proc->sigPipe))
      return errno;

   // Set the read end of the pipe to non-blocking
   currFlags = fcntl(proc->sigPipe[0], F_GETFD);
   if (-1 == currFlags)
      return -1;
   if (-1 == fcntl(proc->sigPipe[0], F_SETFD, O_NONBLOCK))
      return -1;

   signalWriteFD = proc->sigPipe[1];
   return EVT_fd_add(proc->evtHandler, proc->sigPipe[0],
         EVENT_FD_READ, signal_fd_cb, proc);
}

int PROC_signal(struct ProcessData *proc, int sigNum, PROC_signal_cb cb,
            void *p)
{
   struct ProcSignalCB *curr;
   struct sigaction sa;

   if (-1 == signalWriteFD) {
      DBG_print(DBG_LEVEL_WARN, "Failed to add signal because ProcessData"
            " not initialized correctly");
      return -1;
   }

   if (proc->sigPipe[1] != signalWriteFD) {
      DBG_print(DBG_LEVEL_WARN, "Failed to add signal because a different ProcessData"
            " is already catching signals.");
      return -1;
   }

   curr = malloc(sizeof(struct ProcSignalCB));
   if (!curr)
      return -1;

   curr->cb = cb;
   curr->arg = p;
   curr->sigNum = sigNum;
   curr->recvdCnt = 0;
   curr->next = proc->signalCBHead;
   proc->signalCBHead = curr;

   sa.sa_sigaction = PROC_signal_handler;
   sigfillset(&sa.sa_mask); //Catch all signals
   sa.sa_flags = SA_SIGINFO;
   if (-1 == sigaction(sigNum, &sa, NULL)) {
      free(curr);
      return -1;
   }

   return 0;
}

static int next_param(char *str, char **start, char **end)
{
   int inQuote = 0;

   // End of string edge cases
   if (!str) {
      *start = NULL;
      *end = NULL;
      return 0;
   }
   if (!*str) {
      *start = str;
      *end = str;
      return 0;
   }

   // Skip leading whitespace
   *start = str;
   while( isspace(**start) )
      (*start)++;
   if (!**start) {
      *end = *start;
      return 0;
   }
   if (**start == '\'')
      inQuote = 1;
   if (**start == '"')
      inQuote = 2;

   for( *end = *start + 1; *end; (*end)++) {
      if (!**end || (isspace(**end) && !inQuote)) {
         (*end)--;
         return 1;
      }

      if (**end == '\'') {
         if (!inQuote)
            inQuote = 1;
         else if (inQuote == 1)
            inQuote = 0;
      }
      else if (**end == '"') {
         if (!inQuote)
            inQuote = 2;
         else if (inQuote == 2)
            inQuote = 0;
      }
   }

   DBG_print(DBG_LEVEL_WARN, "Should never get here!");
   return 1;
}

static char **parse_cmd(char *cmd)
{
   char *start = cmd, *end;
   int cnt = 0, i;
   char **argv = NULL;
   int len;

   while(next_param(start, &start, &end)) {
      cnt++;
      start = end + 1;
      if (*start)
         start++;
   }

   argv = malloc(sizeof(char*) * (cnt + 1) );
   if (!argv) {
      DBG_print(DBG_LEVEL_WARN, "Failed to allocate memory\n");
      return NULL;
   }
   for (i = 0; i < cnt + 1; i++)
      argv[i] = NULL;

   cnt = 0;
   start = cmd;
   while(next_param(start, &start, &end)) {
      argv[cnt] = start;
      start = end + 1;
      if (*start) {
         *start = 0;
         start++;
      }

      if (argv[cnt][0] == '"' || argv[cnt][0] == '\'') {
         len = strlen(argv[cnt]) - 1;
         if (argv[cnt][0] == argv[cnt][len]) {
            argv[cnt][len] = 0;
            argv[cnt]++;
         }
      }

      cnt++;
   }

   return argv;
}

ProcChild *PROC_fork_child(struct ProcessData *proc, const char *cmdFmt, ...)
{
   va_list ap;
   va_start(ap, cmdFmt);

   ProcChild *child = NULL;
   int in_pipe[2], out_pipe[2], err_pipe[2];

   // Create the I/O pipes
   if (0 != pipe(in_pipe)) {
      DBG_print(DBG_LEVEL_WARN, "stdin pipe failed");
      goto err_cleanup;
   }
   if (0 != pipe(out_pipe)) {
      DBG_print(DBG_LEVEL_WARN, "stdout pipe failed");
      goto err_cleanup;
   }
   if (0 != pipe(err_pipe)) {
      DBG_print(DBG_LEVEL_WARN, "stderr pipe failed");
      goto err_cleanup;
   }
   child = PROC_fork_child_fd(proc, in_pipe[0], in_pipe[1], out_pipe[0], out_pipe[1], err_pipe[0], err_pipe[1], NULL, NULL, cmdFmt, ap);

   close(in_pipe[0]);
   close(out_pipe[1]);
   close(err_pipe[1]);

err_cleanup:
   return child;
}

ProcChild *PROC_fork_child_va(struct ProcessData *proc, int inFd_read, int inFd_write,
      int outFd_read, int outFd_write, int errFd_read, int errFd_write, struct rlimit *cpu_limit,
      struct rlimit *mem_limit, const char *cmdFmt, ...)
{
   va_list ap;
   va_start(ap, cmdFmt);

   return PROC_fork_child_fd(proc, inFd_read, inFd_write, outFd_read, outFd_write, errFd_read,
         errFd_write, cpu_limit, mem_limit, cmdFmt, ap);
}

ProcChild *PROC_fork_child_fd(struct ProcessData *proc, int inFd_read, int inFd_write,
      int outFd_read, int outFd_write, int errFd_read, int errFd_write, struct rlimit *cpu_limit,
      struct rlimit *mem_limit, const char *cmdFmt, va_list ap)
{
   char *cmd = NULL;
   ProcChild *child = NULL;
   char **argv = NULL;
   pid_t childPid;

   if(vasprintf(&cmd, cmdFmt, ap) < 0) {
      ERR_REPORT(DBG_LEVEL_WARN, "vasprintf failure\n");
      goto err_cleanup;
   }

   va_end(ap);

   if (!cmd || !cmd[0]) {
      DBG_print(DBG_LEVEL_WARN, "Bad cmd in fork_child\n");
      goto err_cleanup;
   }

   argv = parse_cmd(cmd);
   if (!argv || !argv[0])
      goto err_cleanup;

   // Fork off the child process
   if ( 0 == (childPid = fork())) {
      // Executing in the child.  Move the pipe FDs into place
      if (0 != close(0))
         exit(1);
      if (0 != close(1))
         exit(2);
      if (0 != close(2))
         exit(3);

      if (inFd_read > -1 && (-1 == dup2(inFd_read, 0)))
         exit(4);
      if (outFd_write > -1 && (-1 == dup2(outFd_write, 1)))
         exit(5);
      if (errFd_write > -1 && (-1 == dup2(errFd_write, 2)))
         exit(6);

      if(inFd_read > -1)
         close(inFd_read);
      if(inFd_write > -1)
         close(inFd_write);
      if(outFd_read > -1)
         close(outFd_read);
      if(outFd_write > -1)
         close(outFd_write);
      if(errFd_read > -1)
         close(errFd_read);
      if(errFd_write > -1)
         close(errFd_write);

      // Move child into own group for signal isolation
      setpgrp();

      // Set memory limits it necessary
      if(mem_limit) {
         if(setrlimit(RLIMIT_AS, mem_limit) == -1) {
            ERRNO_WARN("Failure to set memory limit");
            exit(7);
         }
      }

      // Set cpu limits it necessary
      if(cpu_limit) {
         if(setrlimit(RLIMIT_CPU, cpu_limit) == -1) {
            ERRNO_WARN("Failure to set CPU limit");
            exit(8);
         }
      }

      // Retrieve the current pid for the child
      pid_t childPid = getpid();

      // Set the nice of the child to the default of 0
      if(setpriority(PRIO_PROCESS, childPid, 0) == -1) {
         ERRNO_WARN("Failure to set nice value of child");
         exit(9);
      }

      execvp(argv[0], argv);
      ERRNO_WARN("Exec failed!");
      printf("Exec failure: %s\n", strerror(errno));
      exit(1);
   }

   // The forking failed
   if(childPid == -1)
      goto err_cleanup;

   // Inititate the child
   child = malloc(sizeof(ProcChild));
   if (!child) {
      DBG_print(DBG_LEVEL_WARN, "Failed to allocate memory for ProcChild\n");
      goto err_cleanup;
   }
   memset(child, 0, sizeof(ProcChild));
   child->state = CHILD_STATE_INIT;
   child->parentData = proc;
   child->procId = childPid;

   // Save the file descriptors for stdout, stderr, and stdin
   // incase parent process wants to write to them.
   child->stdin_fd = inFd_write;
   child->stdout_fd = outFd_read;
   child->stderr_fd = errFd_read;

   child->next = proc->childHead;
   proc->childHead = child;

   // Clean up and return the child
err_cleanup:
   if (argv)
      free(argv);
   if (cmd)
      free(cmd);

   return child;
}

// A callback that ignores all data on a FD
static int dump_child_data(int fd, char type, void *arg)
{
   ProcChild *child = (ProcChild*)arg;
   char buff[4096];
   int len;

   len = read(fd, buff, sizeof(buff));
   if (len > 0)
      return EVENT_KEEP;

   if (len < 0)
      ERRNO_WARN("Child read error");
   else
      CHLD_close_fd(child, fd);

   return EVENT_REMOVE;
}

void CHLD_close_stdin(ProcChild *child)
{
   CHLD_close_fd(child, child->stdin_fd);
}

void CHLD_close_fd(ProcChild *child, int fd)
{
   int change = 0;

   if (fd < 0)
      return;

   if (fd == child->stdin_fd) {
      child->stdin_fd = -1;
      change = 1;
   }
   if (fd == child->stdout_fd) {
      child->stdout_fd = -1;
      change = 1;
   }
   if (fd == child->stderr_fd) {
      child->stderr_fd = -1;
      change = 1;
   }

   if (!change)
      return;

   close(fd);

   validate_pipe_flush(child);
}

char CHLD_ignore_stderr(ProcChild *child)
{
   if (!child)
      return 0;
   if (child->stderr_fd < 0)
      return 0;
   if (!child->parentData)
      return 1;

   return EVT_fd_add(child->parentData->evtHandler, child->stderr_fd,
         EVENT_FD_READ, dump_child_data, child);
}

char CHLD_ignore_stdout(ProcChild *child)
{
   if (!child)
      return 0;
   if (child->stdout_fd < 0)
      return 0;
   if (!child->parentData)
      return 1;

   return EVT_fd_add(child->parentData->evtHandler, child->stdout_fd,
         EVENT_FD_READ, dump_child_data, child);
}

char CHLD_death_notice(ProcChild *child, CHLD_death_cb_t deathCb, void *arg)
{
   if (!child)
      return 0;

   child->deathCb = deathCb;
   child->deathArg = arg;

   return 0;
}

static void drain_buffer(ProcChild *child, BufferedStreamState *state,
      int urgent)
{
   int drainLen = 1;
   int i;

   if (!state || !state->cb)
      return;

  while (drainLen > 0 && state->buffLen > 0) {
      drainLen = (*state->cb)(child, urgent, state->arg, state->buff,
            state->buffLen);
      if (drainLen == CHILD_BUFF_ERR)
         break;
      if (drainLen == CHILD_BUFF_STEAL_BUFF) {
         state->buff = NULL;
         state->buffLen = state->buffCap = 0;
         break;
      }
      if (drainLen >= state->buffLen) {
         state->buffLen = 0;
         break;
      }

      // Compress the buffer
      state->buffLen -= drainLen;
      for(i = 0; i < state->buffLen; i++)
         state->buff[i] = state->buff[i + drainLen];
   }
}

static int child_read_data(int fd, char type, void *arg)
{
   ProcChild *child = (ProcChild*)arg;
   BufferedStreamState *state = NULL;
   int readLen;

   if (fd == child->stdout_fd)
      state = &child->streamState[0];
   else if (fd ==  child->stderr_fd)
      state = &child->streamState[1];
   else {
      DBG_print(DBG_LEVEL_WARN, "child_read_data registered for wrong < fd,client>\n");
      return EVENT_REMOVE;
   }

   // Is there any space left in the buffer?  If not, expand it
   if (state->buffLen >= state->buffCap) {
      if (state->buffCap >= READ_BUFF_MAX) {
         drain_buffer(child, state, CHILD_BUFF_NOROOM);
         if (state->buffLen >= state->buffCap) {
            DBG_print(DBG_LEVEL_WARN, "Failed to drain buffer, dropping data!\n");
            state->buffLen = 0;
         }
      }
      if (!state->buff) {
         state->buffCap = READ_BUFF_MIN;
         state->buffLen = 0;
         state->buff = malloc(state->buffCap);
         if (!state->buff) {
            DBG_print(DBG_LEVEL_WARN, "no memory for buffer; very bad!\n");
            exit(1);
         }
      }
      else if (state->buffLen >= state->buffCap &&
            state->buffCap < READ_BUFF_MAX) {
         state->buffCap *= 2;
         if (state->buffCap > READ_BUFF_MAX)
            state->buffCap = READ_BUFF_MAX;
         state->buff = realloc(state->buff, state->buffCap);
         if (!state->buff) {
            DBG_print(DBG_LEVEL_WARN, "no memory for realloc buffer; very bad!\n");
            exit(1);
         }
      }
   }
   assert(state->buffLen < state->buffCap);

   readLen = read(fd, &state->buff[state->buffLen],
         state->buffCap - state->buffLen);

   if (readLen > 0) {
      state->buffLen += readLen;
      drain_buffer(child, state, CHILD_BUFF_NORM);
   }
   else if (readLen == 0) {
      drain_buffer(child, state, CHILD_BUFF_CLOSING);
      if (state->buffLen != 0) {
         DBG_print(DBG_LEVEL_WARN, "Not all bytes read from buffer when closed\n");
      }
      if (state->buff) {
         free(state->buff);
         state->buff = NULL;
         state->buffCap = state->buffLen = 0;
      }
      CHLD_close_fd(child, fd);
      return EVENT_REMOVE;
   } else if (-1 == readLen && errno != EAGAIN) {
      ERRNO_WARN("Error in read, closing");
      CHLD_close_fd(child, fd);
      return EVENT_REMOVE;
   }

   return EVENT_KEEP;
}

char CHLD_stdout_reader(ProcChild *child, CHLD_buf_stream_cb_t cb, void *arg)
{
   if (!child)
      return 0;
   if (child->stdout_fd < 0)
      return 0;
   if (!child->parentData)
      return 1;

   child->streamState[0].cb = cb;
   child->streamState[0].arg = arg;

   return EVT_fd_add(child->parentData->evtHandler, child->stdout_fd,
         EVENT_FD_READ, child_read_data, child);
}

char CHLD_stderr_reader(ProcChild *child, CHLD_buf_stream_cb_t cb, void *arg)
{
   if (!child)
      return 0;
   if (child->stderr_fd < 0)
      return 0;
   if (!child->parentData)
      return 1;

   child->streamState[1].cb = cb;
   child->streamState[1].arg = arg;

   return EVT_fd_add(child->parentData->evtHandler, child->stderr_fd,
         EVENT_FD_READ, child_read_data, child);
}

struct MsgData {
   ProcessData *proc;
   char *data;
   size_t dataLen;
   struct sockaddr_in dest;
};

int socket_write_cb(int fd, char type, void * arg)
{
   // by default, should remove the event
   int retval = EVENT_REMOVE;
   struct MsgData *msg = (struct MsgData *)arg;

   errno = 0;
   socket_write(fd, msg->data, msg->dataLen, &(msg->dest));

   if (errno == 0) {
      free(msg->data);
      free(msg);
   } else if (errno == EAGAIN) {
      // if for some reason it's still trying to block, keep the event
      retval = EVENT_KEEP;
   }

   return retval;
}

int PROC_loopback_cmd(struct ProcessData *proc,
            int cmdNum, void *data, size_t dataLen)
{
   struct sockaddr_in dummy;

   if (cmdNum < 0 || cmdNum >= MAX_NUM_CMDS)
      return -1;

   if (!proc->cmds.cmds[cmdNum].cmd_cb)
      return -2;

   memset(&dummy, 0, sizeof(dummy));
   proc->cmds.cmds[cmdNum].cmd_cb(proc->cmdFd, cmdNum, data, dataLen, &dummy);

   return 0;
}

int PROC_multi_cmd(ProcessData *proc, unsigned char cmd, void *data,
   size_t dataLen)
{
   uint16_t port = socket_multicast_port_by_name(proc->name);
   int retval = 0;
   struct sockaddr_in addr;

   memset((char*)&addr, 0, sizeof(addr));
   addr.sin_addr = socket_multicast_addr_by_name(proc->name);

   if (port > 0 && addr.sin_addr.s_addr) {
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);

      // printf("Sending to %s / %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

      retval = PROC_cmd_sockaddr(proc, cmd, data, dataLen, &addr);
   }

   return retval;
}

static int proc_cmd_internal(ProcessData *proc, int fd, unsigned char cmd, void *data, size_t dataLen, const char *dest)
{
   int port = socket_get_addr_by_name(dest);
   int retval = 0;

   if (port > 0) {
      struct sockaddr_in addr;

      memset((char*)&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);
      addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

      retval = proc_cmd_sockaddr_internal(proc, fd, cmd, data, dataLen, &addr);
   }

   return retval;
}


int PROC_cmd(ProcessData *proc, unsigned char cmd, void *data, size_t dataLen, const char *dest)
{
   return proc_cmd_internal(proc, proc->cmdFd, cmd, data, dataLen, dest);
}

int PROC_cmd_secondary(ProcessData *proc, unsigned char cmd, void *data, size_t dataLen, const char *dest)
{
   return proc_cmd_internal(proc, proc->txFd, cmd, data, dataLen, dest);
}

int PROC_cmd_sockaddr(ProcessData *proc, unsigned char cmd, void *data, size_t dataLen, struct sockaddr_in *dest)
{
   return proc_cmd_sockaddr_internal(proc, proc->cmdFd, cmd, data, dataLen, dest);
}

int PROC_cmd_sockaddr_secondary(ProcessData *proc, unsigned char cmd, void *data, size_t dataLen, struct sockaddr_in *dest)
{
   return proc_cmd_sockaddr_internal(proc, proc->txFd, cmd, data, dataLen, dest);
}

int proc_cmd_sockaddr_internal(ProcessData *proc, int fd, unsigned char cmd, void *data, size_t dataLen, struct sockaddr_in *dest)
{
   int retval = 0;
   struct MsgData *msg = (struct MsgData*)malloc(sizeof(struct MsgData));

   // create buffer large enough to fit data + command
   msg->data = (char*)malloc(dataLen+1);

   // set first byte of data to be the command
   msg->data[0] = cmd;

   // new length is total data + 1 byte for the command
   msg->dataLen = dataLen + 1;

   // copy the data/arguments into the buffer
   memcpy((msg->data)+1, data, dataLen);

   // send the data
   errno = 0;
   retval = socket_write(fd, (void*)(msg->data), msg->dataLen, dest);

   // If it executed error free, it succeeded!
   if (errno == 0) {
      // free temporary data
      free(msg->data);
      free(msg);
   } else if (errno == EAGAIN) {
      // EAGAIN indicates the operation would block
      // DBG("Socket is going to block, FYI!\n");
      msg->proc = proc;
      msg->dest = *dest;
      // schedule a write for when buffer is available
      EVT_fd_add(PROC_evt(proc), fd, EVENT_FD_WRITE, socket_write_cb, (void*)msg);
   }

   return retval;
}

int PROC_set_cmd_handler(struct ProcessData *proc,
            int cmdNum, CMD_handler_t handler, uint32_t uid,
            uint32_t group, uint32_t protection)
{
   if (cmdNum < 0 || cmdNum >= MAX_NUM_CMDS)
      return -1;

   proc->cmds.cmds[cmdNum].cmd_cb = handler;
   proc->cmds.cmds[cmdNum].uid = uid;
   proc->cmds.cmds[cmdNum].group = group;
   proc->cmds.cmds[cmdNum].prot = protection;

   return 0;
}

int PROC_set_multicast_handler(struct ProcessData *proc, const char *service,
      int cmdNum, MCAST_handler_t handler, void *arg)
{
   cmd_set_multicast_handler(&proc->cmds, proc->evtHandler, service, cmdNum,
      handler, arg);

   return 0;
}

static int write_event_callback(int fd, char type, void *arg)
{
   struct ProcessData *proc = (struct ProcessData*)arg;
   struct ProcWriteNode **curr, *tx;

   // Find the next write node for this fd
   for (curr = &proc->writeListHead; *curr && (*curr)->fd != fd;
         curr = &((*curr)->next))
      ;

   // If we were called by mistake (no node on the write list), remove
   // ourselves
   if (!*curr)
      return EVENT_REMOVE;

   // We can finally perform the write
   tx = *curr;
   (tx->cb)(fd, tx->data, tx->len, tx->arg);

   // Cleanup
   *curr = tx->next;
   if (tx->freeMem)
      free(tx->data);
   free(tx);

   // Check to see if there are more packets to transmit out this fd
   while (*curr && (*curr)->fd != fd)
         curr = &((*curr)->next);

   if (!*curr)
      return EVENT_REMOVE;

   return EVENT_KEEP;
}

static void proc_write_callback(int fd, uint8_t *data, uint32_t len, void *arg)
{
   if(write(fd, data, len) < len) {
      ERRNO_WARN("Failed to write callback\n");
   }
}

int PROC_nonblocking_write(struct ProcessData *proc, int fd, uint8_t *data, uint32_t len, int memoryOptions)
{
   return PROC_nonblocking_write_callback(proc, fd, data, len,
         &proc_write_callback, NULL, memoryOptions);
}

int PROC_nonblocking_write_callback(struct ProcessData *proc, int fd, uint8_t *data, uint32_t len, proc_nb_write_cb cb, void *arg, int memoryOptions)
{
   struct ProcWriteNode *newNode;
   struct ProcWriteNode **curr;
   int writePending = 0;

   newNode = malloc(sizeof(*newNode));
   if (!newNode)
      return -1;

   // Set up memory management
   newNode->freeMem = 1;
   if (memoryOptions == IGNORE_DATA_AFTER_WRITE)
      newNode->freeMem = 0;

   newNode->data = NULL;
   newNode->len = 0;
   // Make a copy of the data, if requested
   if (memoryOptions == COPY_DATA_TO_WRITE) {
      newNode->data = malloc(len);
      if (!newNode->data) {
         free(newNode);
         return -1;
      }
      memcpy(newNode->data, data, len);
      newNode->len = len;
   }

   newNode->fd = fd;
   newNode->next = NULL;
   newNode->cb = cb;
   newNode->arg = arg;

   // Find the tail of the list and determine if there is a
   // write pending for this fd
   for (curr = &proc->writeListHead; *curr; curr = &((*curr)->next))
      if ((*curr)->fd == newNode->fd)
         writePending = 1;

   *curr = newNode;

   // Register the write callback
   if (!writePending) {
      if (EVT_fd_add(proc->evtHandler, newNode->fd, EVENT_FD_WRITE,
               &write_event_callback, proc) < 0) {
         *curr = NULL;
         if (newNode->freeMem && newNode->data)
            free(newNode->data);
         free(newNode);
         return -1;
      }
   }

   // Success
   return 0;
}

struct thread_data{
   int write_fd;
   int read_fd;
   pthread_t thread;
   int (*fcn)(void *arg);
   void *fcn_arg;
   int (*cb_fcn)(void *arg, int retval);
   void *cb_arg;
   int retval;
};

void *thread_main(void *arg)
{
   struct thread_data *data = (struct thread_data *)arg;

   data->retval = data->fcn(data->fcn_arg);

   //write data to trigger callback in event loop
   char letter = 'q';
   if(write(data->write_fd, &letter, sizeof(letter)) < sizeof(letter)) {
      ERRNO_WARN("Failed to write callback\n");
   }

   pthread_exit(data);
}

static int thread_cb(int fd, char type, void *data)
{
   struct thread_data *ret = (struct thread_data *)data;

   //join thread
   pthread_join(ret->thread, (void **)&ret);

   ret->cb_fcn(ret->cb_arg, ret->retval);

   //clean up pipe
   close(ret->read_fd);
   close(ret->write_fd);
   free(ret);

   return EVENT_REMOVE;
}

int thread_function(ProcessData *proc, void *fcn_ptr, void *arg, void *cb_fcn,
void *cb_arg)
{
   //wrap data needed for thread in struct
   struct thread_data *data = malloc(sizeof(struct thread_data));
   data->cb_fcn = cb_fcn;
   data->cb_arg = cb_arg;
   pthread_attr_t attr;

   //create pipe
   int fd[2];
   if(pipe(fd)) {
      ERRNO_WARN("Error creating pipe");
      goto cleanup;
   }
   data->write_fd = fd[1];
   data->read_fd = fd[0];

   //wrap info for function to thread in struct
   data->fcn = fcn_ptr;
   data->fcn_arg = arg;

   if (pthread_attr_init(&attr) != 0)
      goto cleanup;
 
   if (pthread_attr_setstacksize(&attr, 0x80000) != 0)
      goto cleanup;

   if(pthread_create(&data->thread, &attr, thread_main, data)){
      //error creating thread
      goto cleanup;
   }

   //register pipe read with event loop
   EVT_fd_add(PROC_evt(proc), data->read_fd, EVENT_FD_READ, &thread_cb, data);

   return 0;
cleanup:
   free(data);
   return 1;
}

