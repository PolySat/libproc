import * as React from "react";

import {
   sprintf
} from 'sprintf-js';

import {
      DebugData
} from "../../debugdata";

export
namespace Events {
   export
   interface Props {
      type: 'fdevents' | 'timedevents';
      fdevents?: DebugData.FDEvent[];
      timedevents?: DebugData.TimedEvent[];
   }
}

export
class Events extends React.Component<Events.Props, {}> {

   render() {
      let name;
      let events;
      if (this.props.type == 'fdevents') {
         name = 'FD Events'
         events = this.props.fdevents.map((e: DebugData.FDEvent) => {
               return <FDEvent key={e.id} event={e}/>
         });
      } else {
         name = 'Timed Events'
         events = this.props.timedevents.map((e: DebugData.TimedEvent) => {
               return <TimedEvent key={e.id} event={e}/>
         });
      }

      return (
         <div className='dbg-events-container'>
            <h3>{name}</h3>
            <div className='dbg-events-cards'>
               {events}
            </div>
         </div>
      );
   }

   private _requestInterval: any;
}

namespace FDEvent {
   export
   interface Props {
      event: DebugData.FDEvent;
   }
}

const FDEvent = (props: FDEvent.Props) => {
   return (
      <div className="dbg-event-card">
         <h4>{props.event.filename}</h4>
         <p>Read Handler: {props.event.read_handler ? props.event.read_handler : 'None'}</p>
         <p>Write Handler: {props.event.write_handler ? props.event.write_handler : 'None'}</p>
         <p>Error Handler: {props.event.error_handler ? props.event.error_handler : 'None'}</p>
      </div>
   );
}

namespace TimedEvent {
   export
   interface Props {
      event: DebugData.TimedEvent;
   }
}

function getTimeStr(time: number) {
   let secs = time % 60;
   let mins = Math.floor(time / 60) % 60;
   let hours = Math.floor(mins / 60);

   let timeStr = sprintf('%02d:%02d', mins, secs);
   if (hours > 0) {
      timeStr = sprintf('%02d:', hours) + timeStr
   }

   return timeStr;
}

const TimedEvent = (props: TimedEvent.Props) => {
   let time = props.event.time_remaining;

   return (
      <div className="dbg-event-card">
         <div className='dbg-timedevent-body'>               
            <h4>{props.event.function}</h4>
            <p>arg: {props.event.arg_pointer}</p>
            {/* <p> Data </p> */}
            {/* <p> Data </p> */}
         </div>                  
         <div className='dbg-timedevent-time'>         
            <h2>{getTimeStr(time)}</h2>
            <p>/{getTimeStr(props.event.event_length)}</p> 
         </div>
      </div>
   );
}