#include "proctest.h"
#include "zhelpers.hpp"
#include "json.hpp"
#include <iostream>
#include <stdio.h>
#include "pseudo_threads.h"
#include <sys/types.h>
#include <signal.h>

using json = nlohmann::json;

zmq::context_t *ProcTest::Proc::Proc::zcontext = NULL;

int ProcTest::ZMQEvent::fd_callback()
{
   int zevents;
   bool remove;
   size_t zevents_len = sizeof(zevents);
   int res = EVENT_KEEP;

   sock.getsockopt(ZMQ_EVENTS, &zevents, &zevents_len);

   while (read_cb && (zevents & ZMQ_POLLIN) ) {
      remove = read_cb(this, EVENT_FD_READ, read_arg) == EVENT_REMOVE;
      if (remove) {
         read_cb = NULL;
         if (!write_cb)
            res = EVENT_REMOVE;
      }

      sock.getsockopt(ZMQ_EVENTS, &zevents, &zevents_len);
   }

   while (write_cb && (zevents & ZMQ_POLLOUT) ) {
      remove = write_cb(this, EVENT_FD_WRITE, write_arg) == EVENT_REMOVE;
      if (remove) {
         write_cb = NULL;
         if (!read_cb)
            res = EVENT_REMOVE;
      }

      sock.getsockopt(ZMQ_EVENTS, &zevents, &zevents_len);
   }

   return res;
}

int ProcTest::Proc::zmq_write_cb(ZMQEvent *evt, char type, void *arg)
{
   Proc *self = (Proc*)arg;
   s_send (evt->socket(), self->command);

   return EVENT_REMOVE;
}

int ProcTest::Proc::zmq_event_read_cb(ZMQEvent *evt, char type, void *arg)
{
   Proc *self = (Proc*)arg;
   uint16_t event;
   uint32_t param __attribute__((unused));

   zmq::message_t msg;
   if (!self->mclient->recv(&msg))
      return EVENT_KEEP;
   if (msg.size() < 6)
      return EVENT_KEEP;

   event = *(uint16_t*)msg.data();
   param = *(uint32_t*) ( ((char*)msg.data()) + 2);
   std::string url = s_recv(*self->mclient);

   if (event == ZMQ_EVENT_CONNECTED) {
      self->connected = true;
   }
   else if (event == ZMQ_EVENT_CLOSED) {
   }
   else if (event == ZMQ_EVENT_MONITOR_STOPPED) {
   }

   return EVENT_KEEP;
}

int ProcTest::Proc::zmq_read_cb(ZMQEvent *evt, char type, void *arg)
{
   Proc *self = (Proc*)arg;

   std::string msg = s_recv (*self->zclient);
   self->SetState(msg);

   return EVENT_KEEP;
}

void ProcTest::Proc::SetState(const std::string &json_str)
{
   state = json::parse(json_str);
   std::cout << "Received: " << state << std::endl;
   if (state["dbg_state"] == "stopped") {
      initialized = true;
      running = false;
      double t = state["current_time"].get<double>();
      tm.tv_sec = t;
      t -= tm.tv_sec;
      tm.tv_usec = 1000000 * t;
      // std::cout << "Stopped at " << j["current_time"] << " " <<
      //        j["gmt_time"] << std::endl;
   }
   else
      running = true;

   if (test)
      test->state_updated(this);
}

int ProcTest::proc_exit(int sig, void *arg)
{
   EventManager *evt = (EventManager*)arg;

   evt->Exit();

   return EVENT_KEEP;
}

ProcTest::ProcTest(const char *procname, enum WatchdogMode wd,
      const char *gbl_clk)
{
   proc = new Process(procname, wd);
   if (proc)
      proc->AddSignalEvent(SIGINT, &proc_exit, proc->event_manager());
   now.tv_sec = now.tv_usec = 0;
   waitingToInitialize = false;
   stepping = false;
}

ProcTest::~ProcTest()
{
   if (pt)
      PT_destroy(pt);
   delete proc;
}

void ProcTest::Run(test_sequence_t seq, void *arg)
{
   sequence = seq;
   seq_arg = arg;
   pt = PT_create(&ProcTest::pt_main, this);
   proc->event_manager()->EventLoop();
   pt = NULL;
}

void ProcTest::pt_main(void *arg)
{
   ProcTest *self = (ProcTest*)arg;

   self->WaitForTestProc();
   if (self->sequence)
      self->sequence(self, self->seq_arg);
   self->ShutdownTestProc();
   self->proc->event_manager()->Exit();
}

void ProcTest::ShutdownTestProc()
{
   if (testProc) {
      testProc->signal(SIGINT);
      testProc->RunProc();
       if (pt) {
          deathEvent = testProc;
          PT_block(pt);
       }
   }
}

void ProcTest::Step()
{
   if (testProc) {
      stepping = true;
      testProc->StepProc();
      PT_block(pt);
      stepping = false;
   }
}

void ProcTest::AdvanceUntil(struct timeval tm)
{
   if (!testProc)
      return;

   if (secondaryProcs.size() == 0) {
      unsigned int ms = 0;
      if (timercmp(&tm, &now, >)) {
         struct timeval diff;
         timersub(&tm, &now, &diff);
         ms = diff.tv_sec * 1000;
         ms += diff.tv_usec / 1000;
      }

      AdvanceForMS(ms);
   }
   else {
      struct timeval now = testProc->Time();

      if (timercmp(&tm, &now, <=))
         return;
      target_time = tm;
      PT_block(pt);
   }
}

void ProcTest::WaitForTestProc()
{
   if (testProc) {
      if (!testProc->isInitialized()) {
         waitingToInitialize = true;
         PT_block(pt);
         waitingToInitialize = false;
      }
   }
}

void ProcTest::state_updated(ProcTest::Proc *p)
{
   if (testProc && p == testProc) {
      now = testProc->Time();
      if (pt &&  target_time.tv_sec) {
         if (timercmp(&now, &target_time, >=))
            PT_unblock(pt);
         else if (!p->isRunning())
            p->StepProc();
      }
      if (waitingToInitialize && testProc->isInitialized() && pt)
         PT_unblock(pt);
      if (stepping && pt)
         PT_unblock(pt);
   }
}

void ProcTest::AdvanceForMS(unsigned int ms)
{
   if (!testProc)
      return;

   if (secondaryProcs.size() == 0) {
      target_time.tv_sec = target_time.tv_usec = 0;
      stepping = true;
      testProc->RunProc(ms);
      if (pt)
         PT_block(pt);
      stepping = false;
   }
   else {
      struct timeval wait = EVT_ms2tv(ms);
      struct timeval tm;

      timeradd(&wait, &now, &tm);
      AdvanceUntil(tm);
   }
}

ProcTest::Proc::Proc(Process *p, const char *executable, const char *name,
      ProcTest *pt, time_t vclkBase)
   : test(pt)
{
   char buff[1024];
   process = p;
   int port = 12345;
   if (!zcontext)
      zcontext = new zmq::context_t(1);

   zclient = new zmq::socket_t(*zcontext, ZMQ_PAIR);
   mclient = new zmq::socket_t(*zcontext, ZMQ_PAIR);

   setenv("LIBPROC_DEBUGGER", "STOPPED", 1);
   if (vclkBase > 0) {
      sprintf(buff, "%ld", vclkBase);
      setenv("LIBPROC_DEBUGGER_VCLK", buff, 1);
   }
   child = PROC_fork_child(process->getProcessData(), "%s", executable);
   unsetenv("LIBPROC_DEBUGGER");
   unsetenv("LIBPROC_DEBUGGER_VCLK");
   // std::cout << "Child is " << child << std::endl;

   running = true;
   initialized = false;
   connected = false;

   CHLD_death_notice(child, &child_death_cb, this);
   CHLD_close_stdin(child);
   // CHLD_ignore_stderr(child);
   // CHLD_ignore_stdout(child);
   CHLD_echo_stderr(child);
   CHLD_echo_stdout(child);

   if (name && name[0]) {
      port = socket_get_addr_by_name(name);
      if (port <= 0)
         port = 12345;
   }

   sprintf(buff, "inproc://monitor-localhost-%d", port);
   assert(0 == zmq_socket_monitor((void*)*zclient, buff, ZMQ_EVENT_ALL));
   mclient->connect(buff);
   mevt = new ProcTest::ZMQEvent(process->event_manager(), *mclient);
   mevt->AddReadEvent(zmq_event_read_cb, this);

   sprintf(buff, "tcp://localhost:%d", port);
   zclient->connect(buff);
   zevt = new ProcTest::ZMQEvent(process->event_manager(), *zclient);
   zevt->AddReadEvent(zmq_read_cb, this);

   tm.tv_sec = tm.tv_usec = 0;
}

ProcTest::Proc::~Proc()
{
   if (connected)
      s_send (*zclient, "{\"command\":\"quit\"}");

   delete mevt;
   delete zevt;
   delete mclient;
   delete zclient;
}

void ProcTest::Proc::child_death_cb(struct ProcChild *child, void *arg)
{
   ProcTest::Proc *self = (ProcTest::Proc*)arg;

   if (child && child == self->child) {
      self->child = NULL;
      self->connected = false;
      if (self->test)
         self->test->child_exited(self);
   }
}

void ProcTest::child_exited(ProcTest::Proc *p)
{
   if (testProc && testProc == p && deathEvent == testProc) {
      deathEvent = NULL;
      if (pt)
         PT_unblock(pt);
      delete testProc;
      testProc = NULL;
   }
}
