#ifndef PROCTEST_H
#define PROCTEST_H

#include <polysat/proclib.h>
#include <list>
#include "json.hpp"
#include <zmq.hpp>

class ProcTest {
   public:
      ProcTest(const char *procname = "test1",
            enum WatchdogMode wd = WD_DISABLED, const char *glb_clk = NULL);
      virtual ~ProcTest();

      class ZMQEvent {
         public:
            typedef int (*ZMQEVT_cb)(ZMQEvent *event, char type, void *arg);

            ZMQEvent(EventManager *e, zmq::socket_t &s)
               : evt(e), sock(s), read_cb(NULL), write_cb(NULL)
            {
               size_t fd_len = sizeof(fd);
               sock.getsockopt(ZMQ_FD, &fd, &fd_len);
            }

            ~ZMQEvent() {
               if (read_cb || write_cb)
                  evt->RemoveReadEvent(fd);
            }

            zmq::socket_t &socket() { return sock; }
            EventManager *events() { return evt; }

            void AddReadEvent(ZMQEVT_cb cb, void *arg = NULL) {
               if (!read_cb && !write_cb)
                  evt->AddReadEvent(fd, &fd_callback_static, this);
               read_cb = cb;
               read_arg = arg;
               fd_callback();
            }

            void RemoveReadEvent() {
               if (read_cb && !write_cb)
                  evt->RemoveReadEvent(fd);
               read_cb = NULL;
            }

            void AddWriteEvent(ZMQEVT_cb cb, void *arg = NULL) {
               if (!write_cb && !read_cb)
                  evt->AddReadEvent(fd, &fd_callback_static, this);
               write_cb = cb;
               write_arg = arg;
               fd_callback();
            }

            void RemoveWriteEvent() {
               if (write_cb && !read_cb)
                  evt->RemoveReadEvent(fd);
               write_cb = NULL;
            }

         private:
            EventManager *evt;
            zmq::socket_t &sock;
            ZMQEVT_cb read_cb, write_cb;
            void *read_arg, *write_arg;
            int fd;

            static int fd_callback_static(int fd, char type, void *arg)
               { return ((ZMQEvent*)arg)->fd_callback(); }
            int fd_callback(void);
      };

      class Proc {
         public:
            Proc(Process *process, const char *executable,
                  const char *name = NULL, ProcTest *test = NULL,
                  time_t vclkBase = 0);
            ~Proc();
            struct timeval Time() { return tm; }
            void SendCmd(const std::string &cmd) {
               command = cmd;
               zevt->AddWriteEvent(&zmq_write_cb, this);
            }

            void StepProc() {
               SendCmd("{\"command\":\"next\"}");
               running = true;
            }

            void RunProc() {
               SendCmd("{\"command\":\"run\"}");
               running = true;
            }

            void RunProc(unsigned int ms) {
               char cmd[256];
               sprintf(cmd, "{\"command\":\"run\",\"ms\":%u}", ms);
               SendCmd(cmd);
               running = true;
            }

            bool isRunning() { return running; }
            bool isInitialized() { return initialized; }
            bool childMatch(struct ProcChild *test) { return test == child; }
            int signal(int num) { return kill(child->procId, num); }

         private:
            Proc(const Proc&);
            const Proc &operator=(const Proc&);
            static void child_death_cb(struct ProcChild *child, void *arg);
            static int zmq_write_cb(ZMQEvent *evt, char type, void *arg);
            static int zmq_read_cb(ZMQEvent *evt, char type, void *arg);
            static int zmq_event_read_cb(ZMQEvent *evt, char type, void *arg);
            void SetState(const std::string &json_str);

            static zmq::context_t *zcontext;
            ProcTest::ZMQEvent *zevt, *mevt;
            zmq::socket_t *zclient;
            zmq::socket_t *mclient;
            Process *process;
            struct ProcChild *child;
            nlohmann::json state;
            struct timeval tm;
            std::string command;
            ProcTest *test;
            bool running, initialized, connected;
      };

      typedef void (*test_sequence_t)(ProcTest *, void *);

      Proc *CreateTestProc(const char *path, const char *name = NULL,
            time_t vclkBase = 0) {
         testProc = new Proc(proc, path, name, this, vclkBase);
         return testProc;
      }

      Proc *CreateSecondaryProc(const char *path, const char *name = NULL,
            time_t vclkBase = 0) {
         Proc *res = new Proc(proc, path, name, this, vclkBase);
         secondaryProcs.push_back(res);
         return res;
      }

      void Run(test_sequence_t seq, void *arg);

      void AdvanceUntil(struct timeval tm);
      void AdvanceForMS(unsigned int ms);
      void Step();
      void WaitForTestProc();
      void KillTestProc();
      void ShutdownTestProc();
      ProcessData *procData() { return proc->getProcessData(); }

   private:
      friend class Proc;

      ProcTest(const ProcTest &);
      const ProcTest &operator=(const ProcTest&);
      static void pt_main(void*);
      static int proc_exit(int sig, void *arg);
      void state_updated(Proc *p);
      void child_exited(Proc *p);

      Process *proc;
      void *pt, *seq_arg;
      test_sequence_t sequence;
      Proc *testProc;
      std::list<Proc*> secondaryProcs;
      struct timeval target_time;
      struct timeval now;
      bool waitingToInitialize, stepping;
      Proc *deathEvent;
};

#endif
