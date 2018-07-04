#include <stdio.h>
#include <polysat/polysat.h>
#include <polysat/eventTimer.h>

#include "../core/eventtimer.h"

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
   printf("Hello World\n");
   return EVENT_KEEP;   
}

int some_crazy_function(void *arg)
{
   return EVENT_KEEP;   
}

int wow(void *arg)
{
   return EVENT_KEEP;   
}

int oh_my(void *arg)
{
   return EVENT_KEEP;   
}

int main(void)
{
   struct ProcessData *proc;
   struct EventTimer *et;

   // Initialize the process
   proc = PROC_init("test1", WD_DISABLED);
   if (!proc) {
      printf("error: failed to initialize process\n");
      return -1;
   }
   
   // Enable the debugger   
   et = ET_debug_init(proc);
   if (!et) {
      printf("error: failed to create event timer\n");
      return -1;
   } 

   // Setup a signal event 
   PROC_signal(proc, SIGINT, &signal_handler, proc);

   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(5000), &my_timed_event, NULL);
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(3000), &some_crazy_function, NULL);
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(10000), &wow, NULL);
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(20000), &oh_my, NULL);

   EVT_start_loop(PROC_evt(proc));
   
   PROC_cleanup(proc);
   return 0;
}
