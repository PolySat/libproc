#include <sys/time.h>
#include <signal.h>
#include "../../events.h"
#include "../../eventTimer.h"
#include "../../proclib.h"
#include "gtest/gtest.h"

namespace {

/**
 * Virtual clock test fixture to test with event loop
 */
class TestEvents : public ::testing::Test {
   
   protected:

      virtual void SetUp() {
         struct timeval t;

         proc = PROC_init(NULL, WD_DISABLED);
         ASSERT_TRUE(proc != NULL);   
      }
      
      virtual void TearDown() {
         PROC_cleanup(proc);
      }

      struct ProcessData *proc;

};

struct HandlerData {
   int count;
   int max;
   struct ProcessData *proc;
};

int handler(void *arg) {
   struct HandlerData *data = (struct HandlerData *)arg;
   if (data->count >= data->max) {
      EVT_exit_loop(PROC_evt(data->proc));
      return EVENT_REMOVE;
   } else {
      data->count++;
      return EVENT_KEEP;
   }
}

// Test running a single timed event
TEST_F(TestEvents, SingleEvent) {
   time_t start, end;
   struct HandlerData data;
   
   data.count = 1;
   data.max = 10;
   data.proc = proc;

   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(100), handler, &data);
   
   start = time(NULL);
   EVT_start_loop(PROC_evt(proc));
   end = time(NULL);

   // Ensure the events took the right amount of time
   EXPECT_GE(end - start, 1);
   
   // Check the event ran the right amount of times
   EXPECT_EQ(data.count, data.max);
}

// Test running a multiple timed events
TEST_F(TestEvents, MultipleEvents) {
   time_t start, end;
   struct HandlerData data1, data2;
   
   data1.count = 0;
   data1.max = 100;
   data1.proc = proc;
   
   data2.count = 1;
   data2.max = 2;
   data2.proc = proc;

   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(100), handler, &data1);
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(1000), handler, &data2);
   
   start = time(NULL);
   EVT_start_loop(PROC_evt(proc));
   end = time(NULL);

   // Ensure the events took the right amount of time
   EXPECT_GE(end - start, 2);
   
   // Check the event ran the right amount of times
   EXPECT_EQ(data1.count, 20);
   EXPECT_EQ(data2.count, data2.max);
}

int sig_handler(int signum, void *arg) {
   struct HandlerData *data = (struct HandlerData *)arg;

   data->count = signum;
   EVT_exit_loop(PROC_evt(data->proc));
}

int emit_signal(void *arg) {
   EXPECT_EQ(0, raise(SIGALRM));
   return EVENT_REMOVE;
}

// Test using signal handler
TEST_F(TestEvents, SignalHandler) {
   time_t start, end;
   struct HandlerData data;
   
   data.count = 0;
   data.max = 0;
   data.proc = proc;
   
   // Add signal handler.
   PROC_signal(proc, SIGALRM, &sig_handler, &data); 
   // Add timed event to trigger signal.
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(1000), emit_signal, NULL);
   
   start = time(NULL);
   EVT_start_loop(PROC_evt(proc));
   end = time(NULL);

   // Ensure the events took the right amount of time
   EXPECT_EQ(end - start, 1);
   
   // Check the event ran the right amount of times
   EXPECT_EQ(data.count, SIGALRM);
}

}

