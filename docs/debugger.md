# Debugging Libproc

libproc provides a socket based interface to allow querying event queue data, setting breakpoints on events, and stepping through events. A ZMQ socket pair exchanges JSON packets to control libproc's execution. libproc maintains the server socket, and users can implement the connecting client socket. Documentation of the various messages and commands follows.

## Enabling the Libproc Debugger

By default, libproc will not enable the debugging interface. To enable the debugger, set an environment variable `LIBPROC_DEBUGGER` to either `"ENABLED"` or `"STOPPED"`. `"ENABLED"` causes libproc to initialize the debug socket but start executing events as normal. `"STOPPED"` also causes the debug socket to be initialized, but event execution is halted until the user send a command to start execution.

## State Report

This JSON packet is sent out by libproc periodically, and it provides the client with the current event queue state. The freqeuncy with which this packet is emitted is configurable. It can be sent out (a) every time libproc passes through the event loop, (b) at some periodic interval, or (c) every time a breakpoint is hit. The packet looks at follows:

```json
{
   "loop_steps": "number of times through the event loop (int)",
   "dbg_state": "current debugger state (<'running', 'stopped'>)",
   "next_step": { "only present if dbg_state is 'stopped'"
      "type": "type of the next event to be executed (<'FD Event', 'Timed Event'>)",
      "id": "unique ID of the next event to be executed (int)"
   },
   "port": "port of debugger socket (int)",
   "timed_events": "number of timed events run (int)",
   "process_name": "name of the libproc process (string)",
   "current_time": "current event timer time (int)",
   "timed_events": [
      {
         "id": "unique way to identify event (int)",
         "function": "function to be called (string)",
         "name": "human readable name of the event (string)",
         "time_remaining": "time until the event will be run (int)",
         "awake_time": "event timer time the event will be executed (int)",
         "scheduled_time": "event timer time the event was placed on the queue (int)",
         "event_length": "the initial delay associated with the event (int)",
         "arg_pointer": "memory address of opaque argument (int)",
         "event_count": "number of timed the event has been executed (int)"
      }
   ],
   "fd_events": [
      {
         "id": "unique way to identify event (int)",
         "read_handler": "optional. function to be called on a read event (string)",
         "write_handler": "optional. function to be called on a write event (string)",
         "error_handler": "optional. function to be called on a error event (string)",
         "filename": "the associated filename (string)",
         "name": "human readable name of the event (string)",
         "time_remaining": "time until the event will be run (int)",
         "awake_time": "event timer time the event will be executed (int)",
         "scheduled_time": "event timer time the event was placed on the queue (int)",
         "event_length": "the initial delay associated with the event (int)",
         "read_breakpoint": "if there is a breakpoint set on read events (boolean)",
         "write_breakpoint": "if there is a breakpoint set on write events (boolean)",
         "error_breakpoint": "if there is a breakpoint set on error event (boolean)",
         "arg_pointer": "memory address of opaque argument (int)",
         "read_count": "number of read events that have occurred (int)",
         "write_count": "number of write events that have occurred (int)",
         "error_count": "number of error events that have occurred (int)",
         "event_count": "number of timed the event has been executed (int)",
         "pausable": "if breakpoints are respected on the file descriptor (boolean)"
      }
   ]
}
```

## Client Commands

These are the commands used to query and change the debugger state. They all follow this general format:

```json
{
   "command": "command name (string)",
   "arbitrary command parameters..."
}
```

### Run

Set debugger state to 'running'. Optionally provide a breakpoint condition.

```json
{
   "command": "run",
   "ms": "optional. milliseconds until the debugger should pause execution (int)."
}
```

### Stop

Set debugger state to 'stopped'.

```json
{
   "command": "stop"
}
```

### Next

Should only be run if debugger is in 'stopped' state. Execute the next event on the queue. The next event to be executed will ALWAYS be the one described by the `next_step` property of the state report JSON packet. Optionally step more than one event.

```json
{
   "command": "next",
   "steps": "optional. the number of events to step (int)."
}
```

### Start Dumping Every Loop

Enable reporting the state for every pass through the event loop. State will not be reported if the event loop is handling an event associated with the debugger.

```json
{
   "command": "start_dumping_every_loop"
}
```

### Stop Dumping Every Loop

Disable reporting the state for every pass through the event loop.

```json
{
   "command": "stop_dumping_every_loop"
}
```

### Dump Now

Force reporting the current event queue state.

```json
{
   "command": "dump_now"
}
```

### Periodic Dump

Report the event queue state periodically after an arbitrary interval. Interval of `0` ms will stop dumps.

```json
{
   "command": "periodic_dump",
   "ms": "milliseconds between state reports. Minimum of 500 ms (int)."
}
```

### Set FD Event Breakpoint

Set a breakpoint on an FD event.

```json
{
   "command": "set_fd_breakpoint",
   "fd": "file descriptor to break on (int)"
}
```

### Clear FD Event Breakpoint

Clear breakpoint on an FD event.

```json
{
   "command": "clear_fd_breakpoint",
   "fd": "file descriptor to clear (int)"
}
```

### Set Timed Event Breakpoint

Set a breakpoint on a timed event.

```json
{
   "command": "set_timed_breakpoint",
   "id": "ID of the timed event to break on (int)",
   "function": "optional. Function name to break on (int)"
}
```

### Clear Timed Event Breakpoint

Clear breakpoint on a timed event.

```json
{
   "command": "clear_timed_breakpoint",
   "id": "optional. ID of the timed event to clear (int)",
   "function": "optional. Function name breakpoint to clear (int)"
}
```