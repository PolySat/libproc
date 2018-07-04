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

int get_function_name(void *func_addr, std::string *name)
{
   Dl_info info;
   
   if (!dladdr(func_addr, &info) || !info.dli_sname) {
      std::cout << "error: did not find function symbol\n";
      return -1;
   }

   name->assign(info.dli_sname);
   return 0;
}

int populate_fd_event(struct EventCB *data, json *js)
{
   const char *fd_path = "/proc/self/fd/";
   char fd_path_buff[20];
   char filename[1024];
   std::string func_name;
   int len;

   sprintf(fd_path_buff, "%s%d", fd_path, data->fd);
   if ((len = readlink(fd_path_buff, filename, 1023)) < 0) {
      std::cout << "error: failed to get filename";
      return -1;
   }
   filename[len] = 0;

   js->emplace("id", (uintptr_t)data);
   js->emplace("filename", std::string(filename));
   js->emplace("fd", data->fd);
   js->emplace("arg_pointer", (uintptr_t)data->arg);    

   if (data->cb[EVENT_FD_READ]) {
      if (get_function_name((void *)data->cb[EVENT_FD_READ], &func_name) < 0)
         return -1;
      js->emplace("read_handler", func_name);
   }

   if (data->cb[EVENT_FD_WRITE]) {
      if (get_function_name((void *)data->cb[EVENT_FD_WRITE], &func_name) < 0)
         return -1;
      js->emplace("write_handler", func_name);
   }

   if (data->cb[EVENT_FD_ERROR]) {
      if (get_function_name((void *)data->cb[EVENT_FD_ERROR], &func_name) < 0)
         return -1;
      js->emplace("error_handler", func_name);
   }

   return 0;
}

int populate_timed_event(ScheduleCB *data, json *js, struct timeval *cur_time)
{
   std::string func_name;

   if (get_function_name((void *)data->callback, &func_name) < 0)
      return -1;

   js->emplace("id", (uintptr_t)data);
   js->emplace("function", std::string(func_name));
   js->emplace("time_remaining", data->nextAwake.tv_sec - cur_time->tv_sec);
   js->emplace("awake_time", data->nextAwake.tv_sec);
   js->emplace("schedule_time", data->scheduleTime.tv_sec);
   js->emplace("event_length", data->timeStep.tv_sec);
   js->emplace("arg_pointer", (uintptr_t)data->arg);

   return 0;
}

int broadcast_debug_data(struct DebugEventTimer *et)
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
   data["port"] = et->proc->cmdPort;
   data["current_time"] = cur_time.tv_sec;
   data["timed_events"] = json::array();
   data["fd_events"] = json::array();   
   
   // TODO: Make loop part of libproc pqueue
   for (size_t i = 1; i <= pqueue_size(q); i++) {
      scurr = (ScheduleCB *)q->d[i];
      js = json({});      
      if (populate_timed_event(scurr, &js, &cur_time) < 0) {
         std::cout << "error: failed to create json structure for timed event\n";
         return -1;
      }
      data["timed_events"].push_back(js);
   }
   
   for (int i = 0; i < ctx->hashSize; i++) {
      ecurr = ctx->events[i];
      if (!ecurr)
         continue;

      for (; ecurr; ecurr = ecurr->next) {
         js = json({});      
         populate_fd_event(ecurr, &js);
         data["fd_events"].push_back(js);
      }
   }

   sdata = data.dump();
   zmq::message_t message(sdata.size());
   memcpy(message.data(), sdata.data(), sdata.size());

   if (!et->pub->send(message)) {
      std::cout << "error: failed to send data over socker\n";
      return -1;
   }

   return 0;
}

int ET_debug_block(struct EventTimer *et, struct timeval *nextAwake,
      ET_block_cb blockcb, void *arg)
{
   struct DebugEventTimer *det = (struct DebugEventTimer *)et;
   struct timeval diffTime, curTime, oneSec = EVT_ms2tv(1000), next;
   int ret; 

   if (broadcast_debug_data((struct DebugEventTimer *)et) < 0)
      std::cout << "error: failed to broadcast debug data\n";
   
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

   if (broadcast_debug_data((struct DebugEventTimer *)et) < 0)
      std::cout << "error: failed to broadcast debug data\n";

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

