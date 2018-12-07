#include <stdio.h>
#include "proclib.h"
#include "events.h"
#include <errno.h>
#include "debug.h"

static struct ProcessData *proc = NULL;

int signal_handler(int signal, void *arg)
{
   struct ProcessData *proc = (struct ProcessData *)arg;

   fprintf(stderr, "\nSignal recieved! Quitting...\n");
   EVT_exit_loop(PROC_evt(proc));

   return 0;
}

/**
 * The event callback function. Will be called once a second.
 */
int my_timed_event(void *arg)
{
   printf("my timed event\n");
   return EVENT_KEEP;   
}

int some_crazy_function(void *arg)
{
   printf("Some Crazy Function\n");
   return EVENT_KEEP;   
}

int wow(void *arg)
{
   printf("Wow!\n");
   return EVENT_KEEP;   
}

int oh_my(void *arg)
{
   printf("Oh My!\n");
   return EVENT_KEEP;   
}

void status_cmd(int socket, unsigned char cmd, void *data,
      size_t dataLen, struct sockaddr_in *fromAddr)
{
   char result = 0;

   printf("Status command!\n");

   PROC_cmd_sockaddr(proc, 0xF1, &result, sizeof(result), fromAddr);
}

int client_read_cb(int fd, char type, void *arg)
{
   char buff[2048];
   int len;

   len = read(fd, buff, sizeof(len));
   if (len > 0)
      write(1, buff, len);
   if (len < 0 && errno == EAGAIN)
      return EVENT_KEEP;
   if (len <= 0) {
      close(fd);
      return EVENT_REMOVE;
   }

   return EVENT_KEEP;
}

int accept_cb(int fd, char type, void *arg)
{
   int client;
   struct sockaddr_in addr;
   socklen_t len = sizeof(addr);

   client = accept(fd, &addr, &len);
   EVT_fd_add(arg, client, EVENT_FD_READ, client_read_cb, arg);
   EVT_fd_set_name(arg, client, "Client %s:%u",
         inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
   write(client, "Hello World!\n", 13);

   return EVENT_KEEP;
}

int main(void)
{
   void *evt;
   int accept_fd;

   // Initialize the process
   proc = PROC_init("test1", WD_DISABLED);
   if (!proc) {
      printf("error: failed to initialize process\n");
      return -1;
   }
   
   DBG_setLevel(DBG_LEVEL_INFO);
   // Setup a signal event 
   PROC_signal(proc, SIGINT, &signal_handler, proc);
   // PROC_set_cmd_handler(proc, 1, &status_cmd, 0, 0, 0);

   evt = EVT_sched_add(PROC_evt(proc), EVT_ms2tv(5000), &my_timed_event, NULL);
   EVT_sched_set_name(evt, "My Timed Event");

   evt = EVT_sched_add(PROC_evt(proc), EVT_ms2tv(3000), &some_crazy_function, NULL);
   EVT_sched_set_name(evt, "Crazy!!");

   evt = EVT_sched_add(PROC_evt(proc), EVT_ms2tv(10000), &wow, NULL);
   EVT_sched_set_name(evt, "WoW");

   evt = EVT_sched_add(PROC_evt(proc), EVT_ms2tv(20000), &oh_my, NULL);
   EVT_sched_set_name(evt, "oH My");

   EVT_set_initial_debugger_state(PROC_evt(proc), EDBG_STOPPED);
   //EVT_set_initial_debugger_state(PROC_evt(proc), EDBG_ENABLED);

   accept_fd = socket_tcp_init(12345);
   EVT_fd_add(PROC_evt(proc), accept_fd, EVENT_FD_READ, accept_cb,
         PROC_evt(proc));
   EVT_fd_set_name(PROC_evt(proc), accept_fd, "TCP Test:12345");

   EVT_start_loop(PROC_evt(proc));
   
   PROC_cleanup(proc);
   return 0;
}
