/* Andrew Puleo's Newbie Project
* 06/01/18
* Stopwatch - starts timing when run, displays minutes and seconds (m:s),
* ctrl-z starts a new lap, pressing return moves time down a line
* */

#include <stdio.h>
#include <time.h>
#include <polysat/polysat.h>
#include <fcntl.h>

int start_min, start_sec;

int timing_event(void *arg){
  struct tm *tmp;
  time_t t;

  tmp = (struct tm*) arg;
  time(&t);
  tmp = localtime(&t);

  /* Print difference between start and current time */
  printf("\rTime %d:%02d\r", tmp->tm_min - start_min, tmp->tm_sec - start_sec);
  fflush(stdout);

  return EVENT_KEEP;
}

int signal_handler_lap(int signal, void *arg){
  struct tm* tmp;
  time_t t;

  tmp = (struct tm*) arg;
  time(&t);
  tmp = localtime(&t);

  printf("\rTime %d:%02d\r", tmp->tm_min - start_min, tmp->tm_sec - start_sec);

  /* Reset start time */
  start_min = tmp->tm_min;
  start_sec = tmp->tm_sec;

  printf("\nTime %d:%02d", tmp->tm_min - start_min, tmp->tm_sec - start_sec);

  return EVENT_KEEP;
}

int signal_handler_end(int signal, void *arg)
{
  struct ProcessData *proc = (struct ProcessData *)arg;

  printf("\n\nSignal recieved! Stopping...\n\n");
  EVT_exit_loop(PROC_evt(proc));

  return 0;
}

int main(void)
{
  // Where liproc stores its state
  struct ProcessData *proc;
  time_t t;
  struct tm *tmp;

  time(&t);
  tmp = localtime(&t);

  start_min = tmp->tm_min;
  start_sec = tmp->tm_sec;

  // Initialize the process
  proc = PROC_init("test1", WD_DISABLED);
  if (!proc) {
    DBG_print(DBG_LEVEL_FATAL, "FAILED TO INITIALIZE PROCESS\n");
    return -1;
  }

  // Setup a scheduled and signal event
  EVT_sched_add(PROC_evt(proc), EVT_ms2tv(200), &timing_event, &tmp);
  PROC_signal(proc, SIGTSTP, &signal_handler_lap, &tmp);
  PROC_signal(proc, SIGINT, &signal_handler_end, proc);

  // Start the event loop
  printf("Timing");
  EVT_start_loop(PROC_evt(proc));

  // Cleanup liproc
  printf("Cleaning up process...\n");
  PROC_cleanup(proc);
  printf("Done.\n");

  return 0;
}
