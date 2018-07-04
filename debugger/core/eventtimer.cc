#include <iostream>
#include <zmq.hpp>
#include <stdlib.h>
#include <sys/select.h>
#include <dlfcn.h>
#include <unistd.h>

#include "../../proclib.h"
#include "../../priorityQueue.h"
#include "../../eventTimer.h"
#include "../../events.h"

#include "json.hpp"
#include "eventtimer.h"

using json = nlohmann::json;

struct DebugEventTimer {
   struct EventTimer et;
   struct EventTimer *real_et;
   struct ProcessData *proc;
   zmq::context_t ctx;
   zmq::socket_t *pub;
};

std::string get_function_name(void *func_addr)
{
   Dl_info info;
   
   if (!dladdr(func_addr, &info) || !info.dli_sname)
      throw std::runtime_error("could not find function symbol");

   return std::string(info.dli_sname);
}

void populate_fd_event(struct EventCB *data, json& js)
{
   const char *fd_path = "/proc/self/fd/";
   char fd_path_buff[32];
   char filename[1024];
   int len;

   sprintf(fd_path_buff, "%s%d", fd_path, data->fd);
   if ((len = readlink(fd_path_buff, filename, 1023)) < 0)
      throw std::runtime_error("failed to get fd_event filename");
   filename[len] = 0;

   js["id"]          = (uintptr_t)data;
   js["filename"]    = std::string(filename);
   js["fd"]          = data->fd;
   js["arg_pointer"] = (uintptr_t)data->arg;    

   if (data->cb[EVENT_FD_READ])
      js["read_handler"] = get_function_name((void *)data->cb[EVENT_FD_READ]);

   if (data->cb[EVENT_FD_WRITE])
      js["write_handler"] = get_function_name((void *)data->cb[EVENT_FD_WRITE]);

   if (data->cb[EVENT_FD_ERROR])
      js["error_handler"] = get_function_name((void *)data->cb[EVENT_FD_ERROR]);
}

void populate_timed_event(ScheduleCB *data, json& js, struct timeval *cur_time)
{
   js["id"]             = (uintptr_t)data;
   js["function"]       = get_function_name((void *)data->callback);
   js["time_remaining"] = data->nextAwake.tv_sec - cur_time->tv_sec;
   js["awake_time"]     = data->nextAwake.tv_sec;
   js["schedule_time"]  = data->scheduleTime.tv_sec;
   js["event_length"]   = data->timeStep.tv_sec;
   js["arg_pointer"]    = (uintptr_t)data->arg;
}

void broadcast_debug_data(struct DebugEventTimer *et)
{
   struct timeval cur_time;
   struct EventState *ctx = PROC_evt(et->proc);
   pqueue_t *q = ctx->queue;
   EventCB *ecurr;   
   ScheduleCB *scurr;
   json data, js;
   std::string sdata;

   et->et.get_monotonic_time(&et->et, &cur_time);

   data["process_name"] = std::string(et->proc->name);
   data["port"]         = et->proc->cmdPort;
   data["current_time"] = cur_time.tv_sec;
   data["timed_events"] = json::array();
   data["fd_events"]    = json::array();   
   
   // TODO: Make loop part of libproc pqueue
   for (size_t i = 1; i <= pqueue_size(q); i++) {
      scurr = (ScheduleCB *)q->d[i];
      js.clear();  
      populate_timed_event(scurr, js, &cur_time);
      data["timed_events"].push_back(json(js));
   }
   
   for (int i = 0; i < ctx->hashSize; i++) {
      ecurr = ctx->events[i];
      if (!ecurr)
         continue;

      for (; ecurr; ecurr = ecurr->next) {
         js.clear();      
         populate_fd_event(ecurr, js);
         data["fd_events"].push_back(json(js));
      }
   }

   sdata = data.dump();
   zmq::message_t message(sdata.size());
   memcpy(message.data(), sdata.data(), sdata.size());

   if (!et->pub->send(message))
      throw std::runtime_error("failed to broadcast debug data");
}

int ET_debug_block(struct EventTimer *et, struct timeval *nextAwake,
      ET_block_cb blockcb, void *arg)
{
   struct DebugEventTimer *det = (struct DebugEventTimer *)et;
   struct timeval diffTime, curTime, oneSec = EVT_ms2tv(1000), next;
   int ret; 

   broadcast_debug_data(det);
   
   et->get_monotonic_time(et, &curTime);
   timeradd(&oneSec, &curTime, &next);
   
   // Use passed next awake time if less than a second away
   if (nextAwake) {
      timersub(nextAwake, &curTime, &diffTime);      
      if (diffTime.tv_sec + (diffTime.tv_usec * 1000000) < 1)
         next = *nextAwake;
   }

   // Call the block function for libproc current EventTimer   
   ret = det->real_et->block(det->real_et, &next, blockcb, arg);

   broadcast_debug_data(det);

   return ret;
}

void ET_debug_cleanup(struct EventTimer *et)
{
   struct DebugEventTimer *det = (struct DebugEventTimer *)et;

   if (!et)
      return;

   if (det->real_et)
      det->real_et->cleanup(det->real_et);

   // Need to close the socket
   //delete det->ctx;
   delete det->pub;
   delete det;
}

int ET_debug_monotonic(struct EventTimer *et, struct timeval *tv)
{
   struct DebugEventTimer *det = (struct DebugEventTimer *)et;

   return det->real_et->get_monotonic_time(det->real_et, tv);
}

int ET_debug_gmt(struct EventTimer *et, struct timeval *tv)
{
   struct DebugEventTimer *det = (struct DebugEventTimer *)et;

   return det->real_et->get_gmt_time(det->real_et, tv);
}

struct EventTimer *ET_debug_init(struct ProcessData *proc)
{
   struct DebugEventTimer *et;

   et = new struct DebugEventTimer();
   
   et->ctx = zmq::context_t(1);
   et->pub = new zmq::socket_t(et->ctx, ZMQ_PUB);
   et->pub->bind("tcp://*:5565");
   et->proc = proc;
   et->real_et = EVT_get_evt_timer(PROC_evt(proc));
   
   et->et.block = &ET_debug_block;
   et->et.cleanup = &ET_debug_cleanup;
   et->et.get_gmt_time = &ET_debug_gmt;
   et->et.get_monotonic_time = &ET_debug_monotonic;
   
   PROC_evt(proc)->evt_timer = (struct EventTimer *)et;

   return (struct EventTimer *)et;
}

