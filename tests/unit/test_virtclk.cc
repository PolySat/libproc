#include <sys/time.h>
#include <signal.h>
#include "../../events.h"
#include "../../virtclk.h"
#include "../../proclib.h"
#include "gtest/gtest.h"

namespace {

/**
 * Basic virtual clock test fixture
 */
class TestVirtClk : public ::testing::Test {
   
   protected:

      virtual void SetUp() {
         struct timeval t1, t2;

         t1 = EVT_ms2tv(20000);
         state = CLK_init(&t1);
         ASSERT_TRUE(state != NULL);

         t2 = CLK_get_time(state);
         EXPECT_EQ(t1.tv_sec, t2.tv_sec);
         EXPECT_EQ(t1.tv_usec, t2.tv_usec);
         EXPECT_EQ(CLK_ACTIVE, CLK_get_pause(state));
      }

      virtual void TearDown() {
         CLK_cleanup(state);
      }

      struct VirtClkState *state;
};

// Test set function
TEST_F(TestVirtClk, Set) {
   struct timeval t1, t2;

   t1 = EVT_ms2tv(20000);
   CLK_set_time(state, &t1);
   t2 = CLK_get_time(state);
   EXPECT_EQ(t1.tv_sec, t2.tv_sec);
   EXPECT_EQ(t1.tv_usec, t2.tv_usec);
   
   t1 = EVT_ms2tv(-200);
   CLK_set_time(state, &t1);
   t2 = CLK_get_time(state);
   EXPECT_EQ(t1.tv_sec, t2.tv_sec);
   EXPECT_EQ(t1.tv_usec, t2.tv_usec);
   
   t1 = EVT_ms2tv(130);
   CLK_set_time(state, &t1);
   t2 = CLK_get_time(state);
   EXPECT_EQ(t1.tv_sec, t2.tv_sec);
   EXPECT_EQ(t1.tv_usec, t2.tv_usec);
   
   t1 = EVT_ms2tv(0);
   CLK_set_time(state, &t1);
   t2 = CLK_get_time(state);
   EXPECT_EQ(t1.tv_sec, t2.tv_sec);
   EXPECT_EQ(t1.tv_usec, t2.tv_usec);
}

// Test increment function
TEST_F(TestVirtClk, Increment) {
   struct timeval t1, t2, t3;

   t1 = CLK_get_time(state);

   t3 = EVT_ms2tv(33213);
   timeradd(&t1, &t3, &t1);
   CLK_inc_time(state, &t3);
   t2 = CLK_get_time(state);
   EXPECT_EQ(t1.tv_sec, t2.tv_sec);
   EXPECT_EQ(t1.tv_usec, t2.tv_usec);
   
   t3 = EVT_ms2tv(200);
   timeradd(&t1, &t3, &t1);
   CLK_inc_time(state, &t3);
   t2 = CLK_get_time(state);
   EXPECT_EQ(t1.tv_sec, t2.tv_sec);
   EXPECT_EQ(t1.tv_usec, t2.tv_usec);
   
   t3 = EVT_ms2tv(40000);
   timeradd(&t1, &t3, &t1);
   CLK_inc_time(state, &t3);
   t2 = CLK_get_time(state);
   EXPECT_EQ(t1.tv_sec, t2.tv_sec);
   EXPECT_EQ(t1.tv_usec, t2.tv_usec);   
}

// Test pause function
TEST_F(TestVirtClk, Pause) {

   CLK_set_pause(state, CLK_PAUSED);
   EXPECT_EQ(CLK_PAUSED, CLK_get_pause(state));
   
   CLK_set_pause(state, CLK_ACTIVE);
   EXPECT_EQ(CLK_ACTIVE, CLK_get_pause(state));
}

/**
 * Virtual clock test fixture to test with event loop
 */
class TestVirtClkEvt : public ::testing::Test {
   
   protected:

      virtual void SetUp() {
         struct timeval t;

         proc = PROC_init(NULL, WD_DISABLED);
         ASSERT_TRUE(proc != NULL);
         
         EVT_enable_virt(PROC_evt(proc), EVT_ms2tv(0));
         EVT_get_gmt_time(PROC_evt(proc), &t);
         EXPECT_EQ(t.tv_sec, 0);
         EXPECT_EQ(t.tv_usec, 0);
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

// Test running a signle event with a virtual clock
TEST_F(TestVirtClkEvt, SingleEvent) {
   struct timeval t;
   time_t start, end;
   struct HandlerData data;
   
   data.count = 1;
   data.max = 5;
   data.proc = proc;

   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(1000), handler, &data);
   
   start = time(NULL);
   EVT_start_loop(PROC_evt(proc));
   end = time(NULL);
   // Ensure the events were run immediately
   EXPECT_LE(end - start, 1);
   
   // Check the event ran the right amount of times
   // and that the virtclk was incremented.
   EXPECT_EQ(data.count, data.max);
   EVT_get_gmt_time(PROC_evt(proc), &t);
   EXPECT_EQ(t.tv_sec, data.max);
   EXPECT_EQ(t.tv_usec, 0);
}

// Test running multiple events with a virtual clock
TEST_F(TestVirtClkEvt, MultipleEvents) {
   struct timeval t;
   time_t start, end;
   struct HandlerData data1, data2;
   
   data1.count = 0;
   data1.max = 100;
   data1.proc = proc;
   
   data2.count = 1;
   data2.max = 2;
   data2.proc = proc;

   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(1000), handler, &data1);
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(5000), handler, &data2);
   
   start = time(NULL);
   EVT_start_loop(PROC_evt(proc));
   end = time(NULL);
   // Ensure the events were run immediately
   EXPECT_LE(end - start, 1);
   
   // Check the event ran the right amount of times
   // and that the virtclk was incremented.
   EXPECT_EQ(data1.count, 10);
   EXPECT_EQ(data2.count, data2.max);
   EVT_get_gmt_time(PROC_evt(proc), &t);
   EXPECT_EQ(t.tv_sec, 10);
   EXPECT_EQ(t.tv_usec, 0);
}

int start_pause(void *arg) {
   struct ProcessData *proc = (struct ProcessData *)arg;
   struct EventState *evt = PROC_evt(proc);
   struct itimerval itv;

   CLK_set_pause(EVT_clk(evt), CLK_PAUSED);

   // Create timer to trigger end of pause after 1 second
   memset(&itv, 0, sizeof(struct itimerval));
   itv.it_value.tv_sec = 1;
   EXPECT_EQ(0, setitimer(ITIMER_REAL, &itv, NULL));
}

int end_pause(int sig, void *arg) {
   struct ProcessData *proc = (struct ProcessData *)arg;
   EVT_exit_loop(PROC_evt(proc));
}


// Test running multiple events with a virtual clock and adding a pause
TEST_F(TestVirtClkEvt, MultipleEventsWithPause) {
   struct timeval t;
   time_t start, end;
   struct HandlerData data;

   // Add a signal handler for when pause timer completes
   PROC_signal(proc, SIGALRM, &end_pause, proc);
   
   data.count = 0;
   data.max = 10;
   data.proc = proc;
   
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(1000), handler, &data);
   // Pause after 5 virtual seconds have passed. Delay for one second
   // then exit loop
   EVT_sched_add(PROC_evt(proc), EVT_ms2tv(5000), start_pause, proc);
   
   start = time(NULL);
   EVT_start_loop(PROC_evt(proc));
   end = time(NULL);
   // Ensure the events were run as expected
   EXPECT_GE(end - start, 1);
   EXPECT_LT(end - start, 2);
   
   // Check the event ran the right amount of times.
   // Should have stopped after pause.
   EXPECT_EQ(data.count, 5);
   EVT_get_gmt_time(PROC_evt(proc), &t);
   EXPECT_EQ(t.tv_sec, 5);
   EXPECT_EQ(t.tv_usec, 0);
}

}

