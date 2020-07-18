/* Andrew Puleo's Newbie Project
* 06/01/18
* Stopwatch - starts timing when run, displays minutes and seconds (m:s),
* ctrl-z starts a new lap, pressing return moves time down a line
* */

#include <stdio.h>
#include <time.h>
#include <polysat3/polysat.h>
#include <fcntl.h>

int start_min, start_sec;

void timer_func(void *arg)
{
   struct EventState *evt = (struct EventState*)arg;

   printf("1\n");
   PT_wait(5000);
   printf("2\n");
   PT_wait(2000);
   printf("3\n");

   if (evt)
      EVT_exit_loop(evt);
}

void read_func(void *arg)
{
   // struct EventState *evt = (struct EventState*)arg;
   int fd;
   ssize_t len;
   char buff[16];

   fd = open("Makefile", O_RDONLY);
   if (fd < 0)
      return;

   while ((len = PT_read(fd, buff, sizeof(buff), 500)) > 0) {
      printf("Read %lu byte\n", len);
      PT_wait(455);
   }
   printf("close!\n");

   close(fd);
}

int main(void)
{
  // Where libproc stores its state
  struct ProcessData *proc;

  // Initialize the process
  proc = PROC_init("test1", WD_DISABLED);
  if (!proc) {
    DBG_print(DBG_LEVEL_FATAL, "FAILED TO INITIALIZE PROCESS\n");
    return -1;
  }

  // Start the event loop
  PT_create(&timer_func, PROC_evt(proc));
  PT_create(&read_func, PROC_evt(proc));
  EVT_start_loop(PROC_evt(proc));

  // Cleanup liproc
  printf("Cleaning up process...\n");
  PROC_cleanup(proc);
  printf("Done.\n");

  return 0;
}
