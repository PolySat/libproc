import './style/application.css';

import * as React from "react";
import * as SockJS from 'sockjs-client';

import {
      DebugData
} from "../debugdata";

import {
   Events
} from './events';

export
namespace Application {
   export
   interface State extends DebugData.Data {}
}

export
class Application extends React.Component<{}, Application.State> {

   constructor(props: any) {
      super(props);
      this.state = {
         process_name: '',
         port: 0,
         current_time: 0,
         timed_events: [],
         fd_events: [],
      }

      this.receiveDebugBroadcast = this.receiveDebugBroadcast.bind(this);
      this._socket = new SockJS('debug-socket');
      this._socket.onmessage = this.receiveDebugBroadcast;;
   }

   componentWillUnmount() {
      clearInterval(this._requestInterval)
   }

   receiveDebugBroadcast(e: any) {
      //TODO: verify data with JSON schema
      this.setState((prevState: Application.State) => {
            let data = e.data as DebugData.Data;

            // Create lookup table of old timed event indexes
            let teIdxs = {} as any;
            prevState.timed_events.forEach((val: DebugData.TimedEvent, idx: number) => {
               teIdxs[val.id] = idx;
            });

            // Sort timed events based on time remaining
            // If times are equal, preserve ordering of prevState.
            data.timed_events = data.timed_events.sort((a: DebugData.TimedEvent, b: DebugData.TimedEvent) => {
               let aOld = teIdxs[a.id];
               let bOld = teIdxs[b.id];

               if (a.time_remaining < b.time_remaining)
                  return -1;
               else if (a.time_remaining > b.time_remaining)
                  return 1;
               else if (aOld === undefined  || bOld === undefined)
                  return 0;
               else if (aOld < bOld)
                  return -1;
               else if (aOld > bOld)
                  return 1;

               return 0;
            });
            return data;
      });
   }

   requestDebugData() {
      fetch('debug')
         .then((resp: Response) => resp.json())
         .catch((error: Error) => {
            console.error('Failed to request data from server!')
         })
         .then((resp: any) => {
            this.setState(resp)
         });
   }

   render() {
      return (
         <div>
         <div className='dbg-header'>
            <div className='dbg-header-spacer'> </div>         
            <h1>libproc Debugger</h1>
         </div>
         <div className='dbg-body'>
            <div className='dbg-body-spacer'> </div>
            <h2>{this.state.process_name}</h2>            
            <div className='dbg-content'>
               <div><Events type='timedevents' timedevents={this.state.timed_events} /></div>
               <div><Events type='fdevents' fdevents={this.state.fd_events} /></div>
            </div>
            <div className="dbg-footer"/>
         </div>
         </div>
      );
   }

   private _requestInterval: any;

   private _socket: WebSocket;
}
