# PolySat Process Library (libproc)

libproc is a lightweight event loop and inter-process communication library built by PolySat.
It powers every one of our satellite core processes!

## Building

libproc is a POSIX C library.
It has been tested on Linux and MacOS, but it will probably run on any unix system.

It can be built with `make` and installed with `make install`.

## Event loop functionality

The event loop allows programs to react to specific events that happen in the operating system.

Some of the event types supported by libproc are:
- File actions: file reading, writing, and errors
- Scheduled events: schedule events to run at specific times
- Incoming commands: React to incoming commands via UDP packets
- Signals: Handle operating system signals, such as `SIGINT`
- Pending power-off

## Inter-process communication

libproc makes it easy to communicate with other processes.
On start-up, libproc reads the process command file definition (`proc_name.cmd.cfg`) and binds to an UDP port.

Using this command definition, other processes can message this process.

In addition to inter-process communication, libproc also supports multicasting,
where multiple processes can subscribe to message streams.

## Open-Source Programs Using libproc

- [ADCS Sensor Reader](https://github.com/PolySat/adcs-sensor-reader): Used to read attitude determination sensors on PolySat missions without full ADCS.

## Hello World using libproc

```c
#include <stdio.h>
#include <polysat/polysat.h>

/*
 * The event callback function. Will be called once a second.
 */
int my_timed_event(void *arg)
{
   printf("Hello World\n");
   /* Returning EVENT_KEEP will reschedule the event to run again on the event loop */
   return EVENT_KEEP;
}

int main(void)
{
   /* Where libproc stores its state */
   struct ProcessData *proc;

   /* Initialize the process */
   proc = PROC_init("test1");
   if (!proc) {
      printf("error: failed to initialize process\n");
      return -1;
   }

   /* Schedule my_timed_event to run once a second on the event loop */
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(1000), &my_timed_event, NULL);

   /* Start the event loop */
   printf("Starting the event loop...\n");
   EVT_start_loop(PROC_evt(proc));

   /* Clean up libproc */
   printf("Cleaning up process...\n");
   PROC_cleanup(proc);

   printf("Done.\n");
   return 0;
}
```

## Additional Examples
[Stopwatch using libproc](https://github.com/PolySat/libproc/tree/master/programs/stopwatch_example)
