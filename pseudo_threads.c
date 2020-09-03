#include "pseudo_threads.h"
#include <ucontext.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "events.h"
#include <errno.h>

#define DEFAULT_STACK_SIZE (1024 * 1024)

struct PThread {
   ucontext_t ctx;
   PT_entry_point_t func;
   void *arg;
   void *stack;
   struct PseudoThreadState *state;
   struct PThread *next;
   struct PThread **head;
   void *sched_evt;
};

struct PseudoThreadState {
   struct PThread *curr;
   struct PThread *ready;
   struct PThread *blocked;
   struct PThread *goner;
   ucontext_t main_ctx;
   struct EventState *evt;
};

static struct PseudoThreadState state;

void PT_init(struct EventState *evt)
{
   memset(&state, 0, sizeof(state));
   state.evt = evt;
   getcontext(&state.main_ctx);
   state.main_ctx.uc_link = &state.main_ctx;
}

static void dequeue_thread(struct PThread *thread)
{
   struct PThread **itr;

   if (!thread || !thread->head)
      return;
   for (itr = thread->head;itr && *itr; itr = &(*itr)->next) {
      if ( (*itr) == thread) {
         *itr = thread->next;
         thread->next = NULL;
         thread->head = NULL;
         break;
      }
   }
}

static void enqueue_thread(struct PThread *self, struct PThread **q)
{
   self->next = *q;
   self->head = q;
   *q = self;
}

int PT_unblock(void *arg)
{
   struct PThread *self = (struct PThread*)arg;

   if (!self)
      return -1;

   dequeue_thread(self);
   enqueue_thread(self, &state.ready);

   return 0;
}


static int timed_evt_cb(void *arg)
{
   struct PThread *self = (struct PThread*)arg;

   if (!self)
      return EVENT_REMOVE;

   self->sched_evt = NULL;
   dequeue_thread(self);
   enqueue_thread(self, &state.ready);

   return EVENT_REMOVE;
}

static int fd_evt_cb(int fd, char type, void *arg)
{
   struct PThread *self = (struct PThread*)arg;

   if (!self)
      return EVENT_REMOVE;

   dequeue_thread(self);
   enqueue_thread(self, &state.ready);

   return EVENT_REMOVE;
}

int PT_block(void *arg)
{
   struct PThread *self = (struct PThread*)arg;

   if (self != state.curr) {
      errno = EINVAL;
      return -1;
   }

   dequeue_thread(self);
   enqueue_thread(self, &state.blocked);
   swapcontext (&self->ctx, &state.main_ctx);
   return 0;
}

static ssize_t pt_rw_int(int fd, void *buff, size_t len, unsigned int timeout,
      ssize_t (*func)(int, void *, size_t))
{
   struct PThread *self;
   ssize_t res;

   if (fd < 0 || !state.evt || !(self = state.curr)) {
      errno = EINVAL;
      return -1;
   }

   if (timeout > 0) {
      assert(self->sched_evt == NULL);
      self->sched_evt = EVT_sched_add(state.evt, EVT_ms2tv(timeout),
         &timed_evt_cb, self);
   }
   do {
      EVT_fd_add(state.evt, fd, EVENT_FD_READ, &fd_evt_cb, self);
      dequeue_thread(self);
      enqueue_thread(self, &state.blocked);
      swapcontext (&self->ctx, &state.main_ctx);
      // Once swapcontext has returned the event triggered
      assert(state.curr == self);
      if (timeout > 0 && self->sched_evt == NULL)
         // We had a timeout!
         res = (ssize_t)-2;
      else {
         if (self->sched_evt) {
            EVT_sched_remove(state.evt, self->sched_evt);
            self->sched_evt = NULL;
         }
         res = func(fd, buff, len);
      }
   } while((ssize_t)-1 == res && errno == EWOULDBLOCK);

   return res;
}

ssize_t PT_read(int fd, void *buff, size_t len, unsigned int timeout)
{
   return pt_rw_int(fd, buff, len, timeout, &read);
}

ssize_t PT_write(int fd, const void *buff, size_t len, unsigned int timeout)
{
   return pt_rw_int(fd, (void*)buff, len, timeout,
         (ssize_t (*)(int, void*, size_t))&write);
}

void PT_wait(unsigned int msecs)
{
   struct PThread *self;
   if (msecs == 0 || !(self = state.curr) || !state.evt)
      return;
   assert(self->sched_evt == NULL);

   self->sched_evt = EVT_sched_add(state.evt, EVT_ms2tv(msecs),
         &timed_evt_cb, self);
   dequeue_thread(self);
   enqueue_thread(self, &state.blocked);
   swapcontext (&self->ctx, &state.main_ctx);
   assert(self == state.curr);
}

static void tmain_wrapper(struct PThread *self)
{
   self->func(self->arg);

   dequeue_thread(self);
   self->state->goner = self;
   if (self->sched_evt) {
      EVT_sched_remove(state.evt, self->sched_evt);
      self->sched_evt = NULL;
   }
}

void *PT_create(PT_entry_point_t tmain, void *arg)
{
   struct PThread *thread;

   thread = (struct PThread*)malloc(sizeof(*thread));
   memset(thread, 0, sizeof(*thread));
   thread->state = &state;
   thread->func = tmain;
   thread->arg = arg;
   thread->stack = malloc(DEFAULT_STACK_SIZE);

   getcontext (&thread->ctx);
   thread->ctx.uc_link = &state.main_ctx;
   thread->ctx.uc_stack.ss_sp = thread->stack;
   thread->ctx.uc_stack.ss_size = DEFAULT_STACK_SIZE;
   makecontext (&thread->ctx, (void (*) (void)) &tmain_wrapper, 1, thread);

   enqueue_thread(thread, &state.ready);

   return thread;
}

void PT_destroy(void *thread)
{
   struct PThread *self = (struct PThread*)thread;

   if (!self || self->state != &state)
      return;

   if (self->sched_evt) {
      EVT_sched_remove(state.evt, self->sched_evt);
      self->sched_evt = NULL;
   }

   if (self == state.curr) {
      dequeue_thread(self);
      state.goner = self;
      swapcontext (&self->ctx, &state.main_ctx);
   }
   else {
      dequeue_thread(self);
      free(self->stack);
      free(self);
   }
}

void PT_run_all(void)
{
   struct PThread *next;

   while ((next = state.ready)) {
      dequeue_thread(next);
      enqueue_thread(next, &state.curr);
      swapcontext (&state.main_ctx, &state.curr->ctx);
      assert(state.curr == NULL);

      if (state.goner) {
         free(state.goner->stack);
         free(state.goner);
         state.goner = NULL;
      }
   }
}
