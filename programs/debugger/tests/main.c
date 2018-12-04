#include <stdio.h>
#include "proclib.h"
#include "events.h"

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

int main(void)
{
   struct ProcessData *proc;

   // Initialize the process
   proc = PROC_init("test1", WD_DISABLED);
   if (!proc) {
      printf("error: failed to initialize process\n");
      return -1;
   }
   
   // Setup a signal event 
   PROC_signal(proc, SIGINT, &signal_handler, proc);

   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(5000), &my_timed_event, NULL);
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(3000), &some_crazy_function, NULL);
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(10000), &wow, NULL);
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(20000), &oh_my, NULL);
   EVT_set_initial_debugger_state(PROC_evt(proc), EDBG_STOPPED);
   //EVT_set_initial_debugger_state(PROC_evt(proc), EDBG_ENABLED);

   EVT_start_loop(PROC_evt(proc));
   
   PROC_cleanup(proc);
   return 0;
}
