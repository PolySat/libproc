export
namespace DebugData {
   export
   interface TimedEvent {
      id: number;
      function: string;
      time_remaining: number;
      awake_time: number;
      schedule_time: number;
      event_length: number;
      arg_pointer: number;
   }

   export
   interface FDEvent {
      id: number;
      read_handler?: string;
      write_handler?: string;
      error_handler?: string;
      filename: string;
      fd: number;
      arg_pointer: number;
   }

   export
   interface Data {
      process_name: string;
      port: number;
      current_time: number;
      timed_events: TimedEvent[];
      fd_events: FDEvent[];
   }
}