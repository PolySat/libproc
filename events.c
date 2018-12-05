/*
 * Copyright PolySat, California Polytechnic State University, San Luis Obispo. cubesat@calpoly.edu
 * This file is part of libproc, a PolySat library.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file events.c Handles events.
 *
 * @author Dr. Bellardo
 * @author Greg Eddington
 */
#include "events.h"
#include "priorityQueue.h"
#include "eventTimer.h"
#include "proclib.h"
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/select.h>
#include <signal.h>
#include <assert.h>
#include <strings.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include "ipc.h"
#include "json.h"

#define EDBG_ENV_VAR "LIBPROC_DEBUGGER"

struct EDBGClient {
   int fd;
   EVTHandler *ctx;
   struct sockaddr_in addr;
   char *rxbuff;
   size_t rxlen, rxmax;
   struct EDBGClient *next;
};

// Static global for virtual time
static EVTHandler *global_evt = NULL;

static void edbg_init(EVTHandler *ctx);
static void edbg_report_state(EVTHandler *ctx);
void evt_fd_set_pausable(EVTHandler *ctx, int fd, char pausable);
extern int ET_default_monotonic(struct EventTimer *et, struct timeval *tv);
extern char EVT_sched_move_to_mono(EVTHandler *handler, void *eventId);
extern void EVT_sched_make_breakpoint(EVTHandler *handler, void *eventId);

#ifndef timercmp
# define timercmp(a, b, CMP)                                                  \
  (((a)->tv_sec == (b)->tv_sec) ?                                             \
   ((a)->tv_usec CMP (b)->tv_usec) :                                          \
   ((a)->tv_sec CMP (b)->tv_sec))
#endif

#ifndef timeradd
#define  timeradd(tvp, uvp, vvp)                \
   do {                       \
      (vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec;     \
      (vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec;  \
      if ((vvp)->tv_usec >= 1000000) {       \
         (vvp)->tv_sec++;           \
         (vvp)->tv_usec -= 1000000;       \
      }                    \
   } while (0)
#endif

#ifndef timersub
#define  timersub(tvp, uvp, vvp)                \
   do {                       \
      (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;     \
      (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;  \
      if ((vvp)->tv_usec < 0) {           \
         (vvp)->tv_sec--;           \
         (vvp)->tv_usec += 1000000;       \
      }                    \
   } while (0)
#endif

#ifndef FD_COPY
#define	FD_COPY(f, t)	bcopy(f, t, sizeof(*(f)))
#endif

// Subracts y timeval struct from x timeval struct and stores the result.
int timeval_subtract(struct timeval *result, struct timeval *x,
	struct timeval *y)
{
  int nsec;

  if (x->tv_usec < 0){
    return 1;
  }
  if (y->tv_usec < 0){
    return 1;
  }

  // Perform the carry for the later subtraction by updating y.
  if (x->tv_usec < y->tv_usec){
    nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000){
    nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  // Compute the time remaining to wait. tv_usec is certainly positive.
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  // Return 1 if result is negative.
  return x->tv_sec < y->tv_sec;
}

// Compare priority callback
static int cmp_pri(struct timeval next, struct timeval curr)
{
	return timercmp(&next, &curr, >=);
}

// Get priority callback
static struct timeval get_pri(void *a)
{
	return ((ScheduleCB *) a)->nextAwake;
}

// Set priority callback
static void set_pri(void *a, struct timeval pri)
{
	((ScheduleCB *) a)->nextAwake = pri;
}

// Get position callback
static size_t get_pos(void *a)
{
	return ((ScheduleCB *) a)->pos;
}

// Set position callback
static void set_pos(void *a, size_t pos)
{
	((ScheduleCB *) a)->pos = pos;
}

int null_evt_callback(void *arg)
{
   return EVENT_REMOVE;
}

/* Initializes an EventState with a given hash size.
 * @param hashSize The hash size of the event handler.
 * @return A pointer to the new EventState
 */
struct EventState *EVT_initWithSize(int hashSize, EVT_debug_state_cb debug_cb,
        void *arg)
{
   struct EventState *res = NULL;
   int i;
   const char *dbg_state;

   res = (struct EventState*)malloc(
         sizeof(struct EventState) + hashSize * sizeof(EventCBPtr));
   if (!res){
      return NULL;
   }

   memset(&res->gpio_intrs, 0, sizeof(res->gpio_intrs));
   res->debuggerStateCB = debug_cb;
   res->debuggerStateArg = arg;

   // Configure the initial debugger state
   dbg_state = getenv(EDBG_ENV_VAR);
   res->initialDebuggerState = EDBG_DISABLED;
   res->debuggerState = EDBG_DISABLED;
   if (dbg_state) {
      if (!strcasecmp(dbg_state, "ENABLED"))
         res->initialDebuggerState = EDBG_ENABLED;
      else if (!strcasecmp(dbg_state, "STOPPED"))
         res->initialDebuggerState = EDBG_STOPPED;
   }

   if (res->initialDebuggerState != EDBG_DISABLED)
      res->debuggerState = EDBG_ENABLED;

   res->keepGoing = 1;
   for ( i = 0; i < EVENT_MAX; i++) {
      FD_ZERO(&res->eventSet[i]);
      FD_ZERO(&res->blockedSet[i]);
      res->maxFds[i] = 0;
      res->eventCnt[i] = 0;
   }
   res->hashSize = hashSize;
   res->maxFd = 0;
   for (i = 0; i < res->hashSize; i++)
      res->events[i] = NULL;

   res->queue = pqueue_init(hashSize, cmp_pri, get_pri, set_pri,
         get_pos, set_pos);
   if (res->queue == NULL){
      free(res);
 	   return NULL;
   }

   // FIXME
   res->dbg_queue = pqueue_init(hashSize, cmp_pri, get_pri, set_pri,
         get_pos, set_pos);
   if (res->dbg_queue == NULL){
      free(res->queue);
      free(res);
 	   return NULL;
   }
   
   res->evt_timer = ET_default_init();
   if (!res->evt_timer) {
      free(res->queue);
      free(res);
      return NULL;
   }
   
   global_evt = res;
   res->loop_counter = 0;
   res->timed_event_counter = 0;
   res->fd_event_counter = 0;
   res->steps_to_pause = 0;
   res->next_timed_event = NULL;
   res->next_fd_event = NULL;
   res->custom_timer = 0;
   res->dump_evt = NULL;
   res->breakpoint_evt = NULL;
   memset(&res->null_evt, 0, sizeof(res->null_evt));
   res->null_evt.callback = null_evt_callback;

   return res;
}

/* Initializes an EventState with a hash size of 19
 * @return A pointer to the new EventState
 */
EVTHandler *EVT_create_handler(EVT_debug_state_cb debug_cb, void *arg)
{
   return (EVTHandler *)EVT_initWithSize(19, debug_cb, arg);
}

static int EVT_remove_internal(struct EventState *ctx, struct EventCB **curr,
      int event)
{
   struct EventCB *tmp;
   int i, deleteIt = 1;

   if (!curr || !*curr || !(*curr)->cb[event]){
      return 0;
   }
   tmp = *curr;

   if (tmp->cleanup[event]){
      (*tmp->cleanup[event])(-1, event, tmp->arg[event]);
   }

   ctx->eventCnt[event]--;
   tmp->cb[event] = NULL;
   tmp->cleanup[event] = NULL;
   tmp->arg[event] = NULL;
   FD_CLR(tmp->fd, &ctx->eventSet[event]);
   FD_CLR(tmp->fd, &ctx->blockedSet[event]);

	if (tmp->fd == ctx->maxFds[event]) {
      ctx->maxFds[event] = 0;;
      if (ctx->eventCnt[event] > 0) {
      	//Find next highest
         for(i = 1; i < tmp->fd; i++)
            if (FD_ISSET(i, &ctx->eventSet[event])){
               ctx->maxFds[event] = i;
            }

         ctx->maxFd = ctx->maxFds[0];
         for (i = 1; i < EVENT_MAX; i++){
            if (ctx->maxFds[i] > ctx->maxFd){
               ctx->maxFd = ctx->maxFds[i];
            }
         }
      }
   }

   for(i = 0; i < EVENT_MAX; i++){
      if (tmp->cb[i]){
         deleteIt = 0;
      }
   }

   if (deleteIt) {
      *curr = tmp->next;
      free(tmp);
   }

   return deleteIt;
}

void EVT_free_handler(EVTHandler *ctx)
{
   int i, event;
   ScheduleCB *curProc;

   if (!ctx)
      return;

   if (ctx->dbgBuffer)
      ipc_destroy_buffer(&ctx->dbgBuffer);

   if (ctx->evt_timer)
      ctx->evt_timer->cleanup(ctx->evt_timer);
   global_evt = NULL;

   if (ctx->breakpoint_evt)
      EVT_sched_remove(ctx, ctx->breakpoint_evt);
   if (ctx->dump_evt)
      EVT_sched_remove(ctx, ctx->dump_evt);

   for(i = 0; i < ctx->hashSize; i++){
      while(ctx->events[i]){
         for(event = 0; event < EVENT_MAX; event++){
            EVT_remove_internal(ctx, &ctx->events[i], event);
         }
      }
   }

   while ((curProc = pqueue_peek(ctx->queue))) {
       pqueue_pop(ctx->queue);
       // Call the callback and see if it wants to be kept
       if (curProc != &ctx->null_evt)
          free(curProc);
   }

   while ((curProc = pqueue_peek(ctx->dbg_queue))) {
       pqueue_pop(ctx->dbg_queue);
       // Call the callback and see if it wants to be kept
       if (curProc != &ctx->null_evt)
          free(curProc);
   }

   pqueue_free(ctx->queue);
   pqueue_free(ctx->dbg_queue);
   free(ctx);
}

void EVT_fd_remove(EVTHandler *ctx, int fd, int event)
{
   struct EventCB **curr;

   for (curr = &ctx->events[fd % ctx->hashSize]; *curr; )
      if ((*curr)->fd == fd) {
         if (!EVT_remove_internal(ctx, curr, event))
            curr = &(*curr)->next;
      }
      else
         curr = &(*curr)->next;
}

char EVT_fd_add(EVTHandler *ctx, int fd, int event, EVT_fd_cb cb, void *p)
{
   return EVT_fd_add_with_cleanup(ctx, fd, event, cb, NULL, p);
}

char EVT_fd_add_with_cleanup(EVTHandler *ctx, int fd, int event,
      EVT_fd_cb cb, EVT_fd_cb cleanup_cb, void *p)
{
   struct EventCB *curr, **currP;
   int i;

   if (!cb) {
      EVT_fd_remove(ctx, fd, event);
      return 0;
   }

   for (currP = &ctx->events[fd % ctx->hashSize];
         *currP && (*currP)->fd != fd;
         currP = &(*currP)->next)
      ;
   curr = *currP;

   if (!curr) {
      curr = (struct EventCB*)malloc(sizeof(struct EventCB));
      if (!curr)
         return -1;

      for (i = 0; i < EVENT_MAX; i++) {
         curr->cb[i] = NULL;
         curr->cleanup[i] = NULL;
         curr->arg[i] = NULL;
         curr->paused[i] = 0;
      }
      curr->pausable = 1;
      curr->fd = fd;
      curr->next = ctx->events[fd % ctx->hashSize];
      ctx->events[fd % ctx->hashSize] = curr;
   }
   if (!curr->cb[event])
      ctx->eventCnt[event]++;
   else {
      fprintf(stderr, "Warning: Only one event handler can be registered for a "
            "fd at a time.  Overwriting event %d on fd %d\n", event, curr->fd);
      if (curr->cleanup[event] && curr->arg[event] != p)
         (*curr->cleanup[event])(-1, event, curr->arg[event]);
   }

   curr->cb[event] = cb;
   curr->cleanup[event] = cleanup_cb;
   curr->arg[event] = p;

   FD_SET(fd, &ctx->eventSet[event]);
   if (!curr->pausable || !curr->paused[event])
      FD_SET(fd, &ctx->blockedSet[event]);

   if (fd > ctx->maxFds[event]){
      ctx->maxFds[event] = fd;
   }

   ctx->maxFd = ctx->maxFds[0];
   for (i = 1; i < EVENT_MAX; i++)
      if (ctx->maxFds[i] > ctx->maxFd)
         ctx->maxFd = ctx->maxFds[i];

   return 1;
}

static int EVT_clean_fdsets(struct EventState *ctx)
{
   fd_set testSet;
   int i, evt, used, res, max;
   struct timeval poll;
   int foundOne = 0;

   DBG_print(DBG_LEVEL_WARN, "Warning: cleaning file descriptors is a slow process. "
         "Remove them when you are done instead!\n");

   max = ctx->maxFd + 1;
   for (i = 0; i < max; i++) {
      used = 0;
      for (evt = 0; evt < EVENT_MAX; evt++)
         used = used || FD_ISSET(i, &ctx->eventSet[evt]);
      if (!used){
         continue;
      }

      FD_ZERO(&testSet);
      FD_SET(i, &testSet);
      poll.tv_sec = 0;
      poll.tv_usec = 0;

      res = select(i + 1, &testSet, NULL, NULL, &poll);
      if (res == -1 && errno == EBADF) {
         foundOne = 1;
         fprintf(stderr, "\tFound fd %d for events: ", i);
         if (FD_ISSET(i, &ctx->eventSet[EVENT_FD_READ]))
            fprintf(stderr, "read ");
         if (FD_ISSET(i, &ctx->eventSet[EVENT_FD_WRITE]))
            fprintf(stderr, "write ");
         if (FD_ISSET(i, &ctx->eventSet[EVENT_FD_ERROR]))
            fprintf(stderr, "error ");
         fprintf(stderr, "\n");

         EVT_fd_remove(ctx, i, EVENT_FD_READ);
         EVT_fd_remove(ctx, i, EVENT_FD_WRITE);
         EVT_fd_remove(ctx, i, EVENT_FD_ERROR);
      }
   }

   return foundOne;
}

/**
 * Enable libproc virtual clock. Sets EventTimer to a new
 * instance of a VirtualEventTimer.
 *
 * @param ctx  EVTHandler struct
 * @param tv   A pointer to the timeval with the desired initial time.
 */
char EVT_enable_virt(EVTHandler *ctx, struct timeval *initTime)
{
   struct EventTimer *et;

   assert(ctx);
 
   et = ET_virt_init(initTime);
   if (!et)
      return -1;

   EVT_set_evt_timer(ctx, et);
   return 0;
}

/**
 * Get current libproc EventTimer.
 *
 * @param ctx  EVTHandler struct
 * @param et   The EventTimer instance.
 */
struct EventTimer *EVT_get_evt_timer(EVTHandler *ctx)
{
   return ctx->evt_timer;
}

/**
 * Set libproc EventTimer.
 *
 * @param ctx  EVTHandler struct
 * @param et   The EventTimer instance.
 */
void EVT_set_evt_timer(EVTHandler *ctx, struct EventTimer *et)
{
   assert(ctx);
   assert(et);

   if (ctx->evt_timer)
      ctx->evt_timer->cleanup(ctx->evt_timer);
   
   ctx->evt_timer = et;
   ctx->custom_timer = 1;
}

/**
 * Get the system's current absolute GMT time.
 *
 * @param tv A pointer to the timeval structure where the time gets stored
 *
 */
int EVT_get_gmt_time(EVTHandler *ctx, struct timeval *tv)
{
   return ctx->evt_timer->get_gmt_time(ctx->evt_timer, tv);
}

/**
 * USE ONLY WHEN ABSOLUTELY NECESSARY!
 *
 * Stateless version of EVT_get_gmt_time. Get the system's current 
 * absolute GMT time without passing a EvtHander context. Use 
 * only when absolutely necessary!
 *
 * @param tv A pointer to the timeval structure where the time gets stored
 */
int EVT_get_gmt_time_virt(struct timeval *tv)
{
   return global_evt->evt_timer->get_gmt_time(global_evt->evt_timer, tv);
}

/**
 * Get the system's current time since an unknown reference point.  This
 * increases monotonically despite any changes to the system's current
 * understanding of GMT.
 *
 * @param tv A pointer to the timeval structure where the time gets stored
 */
int EVT_get_monotonic_time(EVTHandler *ctx, struct timeval *tv)
{
   return ctx->evt_timer->get_monotonic_time(ctx->evt_timer, tv);
}

static void edbg_breakpoint(EVTHandler *ctx)
{
   ctx->debuggerState = EDBG_STOPPED;
   edbg_report_state(ctx);
}

static int evt_process_timed_event(EVTHandler *ctx,
      ScheduleCB *curProc, struct timeval curTime, int stepping)
{
   if (!stepping && (ctx->time_breakpoint || curProc->breakpoint) ) {
      ctx->next_timed_event = curProc;
      edbg_breakpoint(ctx);
      return 0;
   }

   // Pop event from the queue
   ctx->timed_event_counter++;
   curProc->count++;

   // Call the callback and see if it wants to be kept
   if (curProc->callback(curProc->arg) == EVENT_KEEP) {
      curProc->scheduleTime = curTime;
      timeradd(&curProc->nextAwake,
         &curProc->timeStep, &curProc->nextAwake);
      pqueue_insert(curProc->queue, curProc);
   } else {
      if (curProc != &ctx->null_evt) {
         free(curProc);
      }
   }

   return 1;
}

int evt_process_fd_event(EVTHandler *ctx, struct EventCB **evtCurr, int event)
{
   int keep = EVENT_KEEP;

   if ((*evtCurr)->cb[event]) {
      (*evtCurr)->counts[event]++;
      keep = (*(*evtCurr)->cb[event])((*evtCurr)->fd, event,
                        (*evtCurr)->arg[event]);
      ctx->fd_event_counter++;
   }

   if (EVENT_REMOVE == keep)
      EVT_remove_internal(ctx, evtCurr, event);

   return 1;
}

struct EVT_select_cb_args {
   fd_set *eventSetPtrs[EVENT_MAX];
   int maxFd;
   struct timeval *mono_to;
};

static int select_event_loop_cb(struct EventTimer *et,
    struct timeval *nextAwake, void *opaque)
{
   struct EVT_select_cb_args *args = (struct EVT_select_cb_args*)opaque;
   struct timeval *to = nextAwake, now, diff;

   if (args->mono_to) {
      ET_default_monotonic(NULL, &now);
      if (timercmp(&now, args->mono_to, >=))
         diff.tv_sec = diff.tv_usec = 0;
      else
         timersub(args->mono_to, &now, &diff);

      if (!to || timercmp(&diff, to, <))
         to = &diff;
   }

   return select(args->maxFd, args->eventSetPtrs[EVENT_FD_READ],
                  args->eventSetPtrs[EVENT_FD_WRITE],
                  args->eventSetPtrs[EVENT_FD_ERROR], to);
}

char EVT_start_loop(EVTHandler *ctx)
{
   fd_set eventSets[EVENT_MAX];
   struct EVT_select_cb_args args;
   int i;
   int retval;
   int event, fd;
   struct EventCB **evtCurr;
   int startEvent = EVENT_FD_READ;
   int startFd = 0;
   struct timeval curTime, *nextAwake;
   ScheduleCB *curProc;
   int time_paused = 0;
   int fd_paused = 0;
   int real_event;

   ctx->time_breakpoint = ctx->initialDebuggerState == EDBG_STOPPED;
   edbg_init(ctx);

   while(ctx->keepGoing) {
      real_event = 0;
      // Process any single-step events
      if (ctx->dbg_step && ctx->next_timed_event) {
         ctx->evt_timer->get_monotonic_time(ctx->evt_timer, &curTime);
         evt_process_timed_event(ctx, ctx->next_timed_event, curTime, 1);
         ctx->next_timed_event = NULL;
         ctx->debuggerState = EDBG_ENABLED;
         real_event = 1;
      }
      if (ctx->dbg_step && ctx->next_fd_event) {
         evt_process_fd_event(ctx, &ctx->next_fd_event, ctx->next_fd_event_evt);
         ctx->next_fd_event = NULL;
         ctx->debuggerState = EDBG_ENABLED;
         real_event = 1;
      }
      ctx->dbg_step = 0;

      time_paused = fd_paused = ctx->next_timed_event || ctx->next_fd_event;

      for (i = 0; i < EVENT_MAX; i++) {
         if (ctx->eventCnt[i] > 0) {
            args.eventSetPtrs[i] = &eventSets[i];
            if (fd_paused)
				   memcpy(args.eventSetPtrs[i], &ctx->blockedSet[i],
                     sizeof(*(&ctx->blockedSet[i])));
            else
				   memcpy(args.eventSetPtrs[i], &ctx->eventSet[i],
                     sizeof(*(&ctx->eventSet[i])));
         }
         else
            args.eventSetPtrs[i] = NULL;
      }

      args.maxFd = ctx->maxFd + 1;
      args.mono_to = NULL;

      curProc = pqueue_peek(ctx->queue);
      if (!time_paused && curProc)
         nextAwake = &curProc->nextAwake;
      else
         nextAwake = NULL;

      curProc = pqueue_peek(ctx->dbg_queue);
      if (curProc)
         args.mono_to = &curProc->nextAwake;
      
      // Call blocking function of event timer
      retval = ctx->evt_timer->block(ctx->evt_timer, nextAwake, time_paused,
                     &select_event_loop_cb, &args);

      // Process Timed Events
      while (!time_paused && (curProc = pqueue_peek(ctx->queue))) {
         ctx->evt_timer->get_monotonic_time(ctx->evt_timer, &curTime);

         if (timercmp(&curProc->nextAwake, &curTime, >)) {
            // Event is not yet ready
            break;
         }
         pqueue_pop(ctx->queue);
         curProc->pos = SIZE_MAX;
         if (!evt_process_timed_event(ctx, curProc, curTime, 0))
            goto next_loop_iteration;
         real_event = 1;
      }

      while ((curProc = pqueue_peek(ctx->dbg_queue))) {
         ET_default_monotonic(NULL, &curTime);

         if (timercmp(&curProc->nextAwake, &curTime, >)) {
            // Event is not yet ready
            break;
         }
         curProc->pos = SIZE_MAX;
         pqueue_pop(ctx->dbg_queue);
         evt_process_timed_event(ctx, curProc, curTime, 1);
      }

      /* Process FD events */
      if (retval > 0) {
         event = startEvent;
         startFd = (startFd + 1) % args.maxFd;

         do {
            if (args.eventSetPtrs[event]) {
               fd = startFd;
               do {
                  if (FD_ISSET(fd, args.eventSetPtrs[event])) {
                     retval--;
                     for(evtCurr = &ctx->events[fd % ctx->hashSize];
                           *evtCurr; evtCurr = &(*evtCurr)->next) {
                        if ((*evtCurr)->fd != fd)
                           continue;
                        if (!evt_process_fd_event(ctx, evtCurr, event))
                           goto next_loop_iteration;
                        real_event = 1;
                        break;
                     }
                  }
                  fd = (fd + 1) % args.maxFd;
               } while (retval > 0 && fd != startFd);
            }

            event = (event + 1) % EVENT_MAX;
         } while (retval > 0 && event != startEvent);
         startEvent = (startEvent + 1) % EVENT_MAX;
      }

      /* Recover from a select few errors.  Stop the event loop and
         gripe for all others */
      else if (retval == -1) {
         if (errno == 0 || errno == EINTR) {
            ctx->loop_counter++;
            continue;
         }
         else if (errno == EBADF) {
            if (!EVT_clean_fdsets(ctx)) {
               errno = EBADF;
               perror("Unrecoverable error in EVT_loop");
               return -1;
            }
         } else {
            perror("Unrecoverable error in EVT_loop");
            return -1;
         }
      }

next_loop_iteration:
      ctx->loop_counter++;

      if (real_event)
         ;// edbg_report_state(ctx);
   }

   return 0;
}

/**
 * Add a scheduled event callback.
 *
 * @param handler The event handler.
 * @param time The time when the event should occur.
 * @param cb The event callback.
 * @param arg The callback argument.
 *
 * @return An unique identifier for the scheduled event or NULL in the
 *   case of a failure.  This identifier is necessary to change the event later.
 */
void *EVT_sched_add(EVTHandler *handler, struct timeval time,
      EVT_sched_cb cb, void *arg)
{
   ScheduleCB *newSchedCB;

   newSchedCB = malloc(sizeof(ScheduleCB));
   if (!newSchedCB){
     return NULL;
   }

   handler->evt_timer->get_monotonic_time(handler->evt_timer, &newSchedCB->scheduleTime);
   newSchedCB->timeStep = time;
   timeradd(&newSchedCB->scheduleTime, &time, &newSchedCB->nextAwake);
   newSchedCB->callback = cb;
   newSchedCB->arg = arg;
   newSchedCB->queue = handler->queue;

   if (0 == pqueue_insert(newSchedCB->queue, newSchedCB)){
     return newSchedCB;
   }

   free(newSchedCB);
   return NULL;
}

/**
 * Add a scheduled event callback with a specified timestep.
 *
 * @param handler The event handler.
 * @param time The time when the event should occur.
 * @param timestep The time step of when the event should be rescheduled.
 * @param cb The event callback.
 * @param arg The callback argument.
 *
 * @return An unique identifier for the scheduled event or NULL in the
 *   case of a failure.  This identifier is necessary to change the event later.
 */
void *EVT_sched_add_with_timestep(EVTHandler *handler, struct timeval time,
      struct timeval timestep, EVT_sched_cb cb, void *arg)
{
   ScheduleCB *newSchedCB;

   newSchedCB = malloc(sizeof(ScheduleCB));
   if (!newSchedCB)
      return NULL;

   handler->evt_timer->get_monotonic_time(handler->evt_timer, &newSchedCB->scheduleTime);
   newSchedCB->timeStep = timestep;
   timeradd(&newSchedCB->scheduleTime, &time, &newSchedCB->nextAwake);
   newSchedCB->callback = cb;
   newSchedCB->arg = arg;
   newSchedCB->queue = handler->queue;

   if (0 == pqueue_insert(newSchedCB->queue, newSchedCB)){
      return newSchedCB;
   }

   free(newSchedCB);
   return NULL;
}

/**
 * Remove a scheduled event.
 *
 * @param handler The event handler.
 * @param event The event to remove.
 *
 * @return 0 on success, other value if it is not found
 */
void *EVT_sched_remove(EVTHandler *handler, void *eventId)
{
   ScheduleCB *evt = (ScheduleCB*)eventId;
   void *result = NULL;
   if (!evt)
      return NULL;

   if (handler->next_timed_event == evt)
      handler->next_timed_event = &handler->null_evt;

   if (SIZE_MAX == evt->pos) {
      result = evt->arg;
      if (evt != &handler->null_evt)
         free(evt);
   }
   else if (0 == pqueue_remove(evt->queue, eventId)) {
      evt->pos = SIZE_MAX;
      result = evt->arg;
      if (evt != &handler->null_evt)
         free(evt);
   }

   return result;
}

/**
 * Update event callback time.  The new full time will elapse before
 *   the callback is called.
 *
 * @param handler The event handler.
 * @param event The event to change.
 * @param time The time until the event should occur.
 *
 * @return 0 on success, other value if it is not found
 */
char EVT_sched_update(EVTHandler *handler, void *eventId, struct timeval time)
{
   ScheduleCB *evt = (ScheduleCB*)eventId;

   if (!evt){
      return 1;
   }

   if (evt->queue == handler->queue)
      handler->evt_timer->get_monotonic_time(handler->evt_timer,
            &evt->scheduleTime);
   else
      ET_default_monotonic(NULL, &evt->scheduleTime);

   timeradd(&evt->scheduleTime, &time, &evt->nextAwake);
   evt->timeStep = time;
   pqueue_change_priority(evt->queue, evt->nextAwake, evt);

   return 0;
}

void EVT_sched_make_breakpoint(EVTHandler *handler, void *eventId)
{
   ScheduleCB *evt = (ScheduleCB*)eventId;

   if (!evt)
      return;

   evt->breakpoint = 1;
}

char EVT_sched_move_to_mono(EVTHandler *handler, void *eventId)
{
   ScheduleCB *evt = (ScheduleCB*)eventId;

   if (!evt){
      return 1;
   }

   pqueue_remove(evt->queue, eventId);
   ET_default_monotonic(NULL, &evt->scheduleTime);
   timeradd(&evt->scheduleTime, &evt->timeStep, &evt->nextAwake);
   evt->queue = handler->dbg_queue;
   pqueue_insert(evt->queue, evt);

   return 0;
}

/**
 * Update event callback time.  The callback is given credit for the time
 *   time it has already waited, potentially being called immediately.
 *
 * @param handler The event handler.
 * @param event The event to change.
 * @param time The time until the event should occur.
 *
 * @return 0 on success, other value if it is not found
 */
char EVT_sched_update_partial_credit(EVTHandler *handler, void *eventId,
     struct timeval time)
{
   ScheduleCB *evt = (ScheduleCB*)eventId;
   struct timeval now;

   if (!evt)
      return 1;

   timeradd(&evt->scheduleTime, &time, &evt->nextAwake);

   if (evt->queue == handler->queue)
      handler->evt_timer->get_monotonic_time(handler->evt_timer, &now);
   else
      ET_default_monotonic(NULL, &now);

   if (timercmp(&evt->nextAwake, &now, <=))
      evt->nextAwake = now;

   evt->timeStep = time;
   pqueue_change_priority(evt->queue, evt->nextAwake, evt);

   return 0;
}

void EVT_exit_loop(EVTHandler *ctx)
{
   ctx->keepGoing = 0;
}

/**
  * Convert a millisecond value into a timeval suitable for registering
  * events.
  *
  * @param ms milliseconds
  * @return The corresponding timeval structure
  */
struct timeval EVT_ms2tv(int ms)
{
   struct timeval res;

   res.tv_sec = ms / 1000;
   res.tv_usec = (ms - res.tv_sec * 1000)*1000;

   return res;
}

static int add_gpio_intr_callback(struct GPIOInterruptDesc *intr,
      EVT_sched_cb cb, void *arg)
{
   struct GPIOInterruptCBList *newNode;

   if (!intr->tripped) {
      newNode = malloc(sizeof(*newNode));
      if (!newNode)
         return -ENOMEM;

      newNode->cb = cb;
      newNode->arg = arg;
      newNode->next = intr->callbacks;
      intr->callbacks = newNode;
   }
   else
      (*cb)(arg);

   return 0;
}

static int remove_gpio_intr_callback(struct GPIOInterruptDesc *intr,
      EVT_sched_cb cb, void *arg)
{
   struct GPIOInterruptCBList **curr;

   for (curr = &intr->callbacks; curr && *curr; ) {
      struct GPIOInterruptCBList *goner = *curr;
      if (goner->cb == cb && goner->arg == arg) {
         *curr = goner->next;
         free(goner);
      }
      else
         curr = &(*curr)->next;
   }

   return 0;
}

static int trip_gpio_intr_callback(int fd, char type, void *arg)
{
   struct GPIOInterruptDesc *intr = (struct GPIOInterruptDesc*)arg;
   char buff[2];
   int res;

   if (sizeof(buff) != (res = read(intr->fd, buff, sizeof(buff)))) {
      return EVENT_KEEP;
   }

   if (buff[0] != 'Y')
      return EVENT_KEEP;

   intr->tripped = 0x123456;
   if (intr->fd > 0) {
      close(intr->fd);
      intr->fd = -1;
   }

   while (intr->callbacks) {
      struct GPIOInterruptCBList *curr = intr->callbacks;

      intr->callbacks = curr->next;
      if (curr->cb)
         (*curr->cb)(curr->arg);

      free(curr);
   }

   return EVENT_REMOVE;
}

static int setup_gpio_intr(EVTHandler *handler, struct GPIOInterruptDesc *intr, const char *path)
{
   if (!intr)
      return -1;
   if (intr->tripped || intr->fd > 0)
      return 0;

   intr->filename = path;
   intr->tripped = 0;
   intr->fd = 0;
   intr->callbacks = NULL;

   intr->fd = open(path, O_RDONLY);
   if (intr->fd < 0)
      return -errno;

   if (!EVT_fd_add(handler, intr->fd, EVENT_FD_READ,
         trip_gpio_intr_callback, intr)) {
      close(intr->fd);
      intr->fd = 0;
   }

   return 0;
}

int EVT_add_ppod_deployment_cb(EVTHandler *handler, EVT_sched_cb cb, void *arg)
{
   int ret;

   ret = setup_gpio_intr(handler, &handler->gpio_intrs[0],
            "/dev/deployment_switch");
   if (ret < 0)
      return ret;

   return add_gpio_intr_callback(&handler->gpio_intrs[0], cb, arg);
}

int EVT_remove_ppod_deployment_cb(EVTHandler *handler, EVT_sched_cb cb, void *arg)
{
   return remove_gpio_intr_callback(&handler->gpio_intrs[0], cb, arg);
}

int EVT_add_pending_reboot_cb(EVTHandler *handler, EVT_sched_cb cb, void *arg)
{
   int ret;

   ret = setup_gpio_intr(handler, &handler->gpio_intrs[1],
            "/dev/reboot_pending");
   if (ret < 0)
      return ret;

   return add_gpio_intr_callback(&handler->gpio_intrs[1], cb, arg);
}

int EVT_remove_pending_reboot_cb(EVTHandler *handler, EVT_sched_cb cb, void *arg)
{
   return remove_gpio_intr_callback(&handler->gpio_intrs[1], cb, arg);
}

void EVT_set_initial_debugger_state(EVTHandler *handler,
      enum EVTDebuggerState st)
{
   handler->initialDebuggerState = st;
   if (handler->initialDebuggerState != EDBG_DISABLED)
      handler->debuggerState = EDBG_ENABLED;
}

int edbg_breakpoint_cb(void *arg)
{
   EVTHandler *ctx = (EVTHandler*)arg;
   ctx->breakpoint_evt = NULL;
   return EVENT_REMOVE;
}

static int edbg_dump_cb(void *arg)
{
   EVTHandler *ctx = (EVTHandler*)arg;

   edbg_report_state(ctx);

   return EVENT_KEEP;
}

static int edbg_client_msg(struct ZMQLClient *client, const void *data,
      size_t dataLen, void *arg)
{
   EVTHandler *ctx = (EVTHandler*)arg;
   char *cmd = NULL;
   int steps;

   if (json_get_string_prop(data, dataLen, "command", &cmd) < 0)
      return 0;

   if (!strcasecmp(cmd, "run")) {
      ctx->time_breakpoint = 0;
      ctx->dbg_step = 1;
      if (ctx->breakpoint_evt)
         EVT_sched_remove(ctx, ctx->breakpoint_evt);
      ctx->breakpoint_evt = NULL;

      json_get_int_prop(data, dataLen, "ms", &steps);
      if (steps >= 10) {
         ctx->breakpoint_evt = EVT_sched_add(ctx, EVT_ms2tv(steps),
               &edbg_breakpoint_cb, ctx);
         EVT_sched_make_breakpoint(ctx, ctx->breakpoint_evt);
      }
   }
   else if (!strcasecmp(cmd, "stop"))
      ctx->time_breakpoint = 1;
   else if (!strcasecmp(cmd, "next")) {
      ctx->steps_to_pause = 1;
      ctx->time_breakpoint = 1;
      ctx->dbg_step = 1;
      if (json_get_int_prop(data, dataLen, "steps", &steps) >= 0) {
         ctx->steps_to_pause = steps;
      }
   }
   else if (!strcasecmp(cmd, "periodic_dump")) {
      steps = 0;
      json_get_int_prop(data, dataLen, "ms", &steps);
      if (ctx->dump_evt)
         EVT_sched_remove(ctx, ctx->dump_evt);
      ctx->dump_evt = NULL;

      if (steps >= 500) {
         ctx->dump_evt = EVT_sched_add(ctx, EVT_ms2tv(steps),
               &edbg_dump_cb, ctx);
         EVT_sched_move_to_mono(ctx, ctx->dump_evt);
      }
   }
   else {
      printf("Unknown JSON command: %s in:\n", cmd);
      steps = write(1, data, dataLen);
      printf("\n");
   }

   if (cmd)
      free(cmd);

   return 0;
}

static void edbg_debugger_connect(struct ZMQLClient *client, void *arg)
{
   EVTHandler *ctx = (EVTHandler*)arg;

   edbg_report_state(ctx);
}

static void edbg_debugger_disconnect(struct ZMQLClient *client, void *arg)
{
   EVTHandler *ctx = (EVTHandler*)arg;

   if (0 == zmql_client_count(zmql_server_for_client(client))) {
      if (ctx->dump_evt)
         EVT_sched_remove(ctx, ctx->dump_evt);
      ctx->dump_evt = NULL;
   }
}

static void edbg_init(EVTHandler *ctx)
{
   struct EventTimer *et;

   if (ctx->initialDebuggerState == EDBG_DISABLED || !ctx->dbgPort)
      return;

   if (!ctx->custom_timer) {
      et = ET_rtdebug_init();
      if (et) {
         if (ctx->evt_timer)
            ctx->evt_timer->cleanup(ctx->evt_timer);
   
         ctx->evt_timer = et;
      }
   }

   ctx->dbgServer = zmql_create_tcp_server(ctx, ctx->dbgPort,
         edbg_client_msg, &edbg_debugger_connect, &edbg_debugger_disconnect,
         ctx);
   if (!ctx->dbgServer)
      return;

   ctx->dbgBuffer = ipc_alloc_buffer();
   evt_fd_set_pausable(ctx, zmql_server_socket(ctx->dbgServer), 0);
}

static const char *get_function_name(void *func_addr)
{
   Dl_info info;

   if (!dladdr(func_addr, &info)) {
      perror("dladdr");
      return "";
   }

   if (!info.dli_sname)
      return "";

   return info.dli_sname;
}

static void edbg_report_timed_event(struct IPCBuffer *json, ScheduleCB *data,
         struct timeval *cur_time, int first)
{
   struct timeval remain;
   const char *rem_sign = "";

   if (timercmp(&data->nextAwake, cur_time, >=))
      timersub(&data->nextAwake, cur_time, &remain);
   else {
      rem_sign = "-";
      timersub(cur_time, &data->nextAwake, &remain);
   }

   if (!first)
      ipc_printf_buffer(json,
         ",\n");

   ipc_printf_buffer(json,
         "    {\n"
         "      \"id\":%lu,\n"
         "      \"function\":\"%s\",\n"
         "      \"time_remaining\":%s%ld.%06ld,\n"
         "      \"awake_time\":%ld.%06ld,\n"
         "      \"schedule_time\":%ld.%06ld,\n"
         "      \"event_length\":%ld.%06ld,\n"
         "      \"arg_pointer\":%lu,\n"
         "      \"event_count\":%u\n"
         "    }",
         (uintptr_t)data, get_function_name((void *)data->callback),
         rem_sign, remain.tv_sec, remain.tv_usec, data->nextAwake.tv_sec,
         data->nextAwake.tv_usec, data->scheduleTime.tv_sec,
         data->scheduleTime.tv_usec, data->timeStep.tv_sec,
         data->timeStep.tv_usec, (uintptr_t)data->arg, data->count);
}

static void edbg_report_timed_events(struct IPCBuffer *json, EVTHandler *ctx,
         struct timeval *cur_time)
{
   size_t i;
   int first = 1;

   ipc_printf_buffer(json, "  \"timed_events\": [\n");

   if (ctx->next_timed_event) {
      edbg_report_timed_event(json, ctx->next_timed_event, cur_time, first);
      first = 0;
   }

   for (i = 1; i <=  pqueue_size(ctx->queue); i++) {
      edbg_report_timed_event(json, (ScheduleCB *)ctx->queue->d[i],
            cur_time, first);
      first = 0;
   }
   ipc_printf_buffer(json, "\n  ],\n");
}

static void edbg_report_fd_event(struct IPCBuffer *json, struct EventCB *data,
      int first)
{
   char fd_path_buff[32];
   char filename[1024];
   int len;

   sprintf(fd_path_buff, "/proc/self/fd/%d", data->fd);
   if ((len = readlink(fd_path_buff, filename, 1023)) < 0)
      return;
   filename[len] = 0;

   if (!first)
      ipc_printf_buffer(json,
         ",\n");

   ipc_printf_buffer(json,
         "    {\n"
         "      \"id\":%lu,\n"
         "      \"filename\":\"%s\",\n"
         "      \"arg_pointer\":%lu,\n",
         (uintptr_t)data, filename, (uintptr_t)data->arg);

   if (data->cb[EVENT_FD_READ])
      ipc_printf_buffer(json, "      \"read_handler\":\"%s\",\n",
            get_function_name(data->cb[EVENT_FD_READ]));

   if (data->cb[EVENT_FD_WRITE])
      ipc_printf_buffer(json, "      \"write_handler\":\"%s\",\n",
            get_function_name(data->cb[EVENT_FD_WRITE]));

   if (data->cb[EVENT_FD_ERROR])
      ipc_printf_buffer(json, "      \"error_handler\":\"%s\",\n",
            get_function_name(data->cb[EVENT_FD_ERROR]));

   ipc_printf_buffer(json,
         "      \"pausable\":%d,\n"
         "      \"read_blocked\":%d,\n"
         "      \"write_blocked\":%d,\n"
         "      \"error_blocked\":%d,\n"
         "      \"read_count\":%u,\n"
         "      \"write_count\":%u,\n"
         "      \"error_count\":%u,\n"
         "      \"fd\":%d\n"
         "    }", data->pausable, data->paused[EVENT_FD_READ],
         data->paused[EVENT_FD_WRITE], data->paused[EVENT_FD_ERROR],
         data->counts[EVENT_FD_READ], data->counts[EVENT_FD_WRITE],
         data->counts[EVENT_FD_ERROR], data->fd);
}

static void edbg_report_fd_events(struct IPCBuffer *json, EVTHandler *ctx)
{
   int i, first = 1;
   struct EventCB *curr;

   ipc_printf_buffer(json, "  \"fd_events\": [\n");

   for (i = 0; i < ctx->hashSize; i++)
      for (curr = ctx->events[i]; curr; curr = curr->next) {
         edbg_report_fd_event(json, curr, first);
         first = 0;
      }

   if (!first)
      ipc_printf_buffer(json, "\n");
   ipc_printf_buffer(json, "  ],\n");
}

static void edbg_report_state(EVTHandler *ctx)
{
   struct timeval curr_time;

   if (!ctx || !ctx->dbgServer || !ctx->dbgBuffer)
      return;
   if (zmql_client_count(ctx->dbgServer) == 0)
      return;

   EVT_get_monotonic_time(ctx, &curr_time);

   // Fill the buffer with state information
   ipc_reset_buffer(ctx->dbgBuffer);
   ipc_printf_buffer(ctx->dbgBuffer,
         "{\n  \"loop_steps\": %llu,\n  \"dbg_state\": \"%s\",\n  "
         "\"port\":%u,\n  \"steps\":%llu,\n", ctx->loop_counter, 
         ctx->debuggerState == EDBG_STOPPED ? "stopped" : "running",
         ctx->dbgPort, ctx->timed_event_counter);

   if (ctx->debuggerStateCB)
      ctx->debuggerStateCB(ctx->dbgBuffer, ctx->debuggerStateArg);

   if (ctx->debuggerState == EDBG_STOPPED) {
      if (ctx->next_timed_event)
         ipc_printf_buffer(ctx->dbgBuffer,
            "  \"next_step\" : { \"type\":\"Timed Event\", \"id\":%lu },\n",
            (uintptr_t)ctx->next_timed_event );
      else if (ctx->next_fd_event) {
         ipc_printf_buffer(ctx->dbgBuffer,
            "  \"next_step\" : { \"type\":\"FD Event\", \"id\":%lu },\n",
            (uintptr_t)ctx->next_fd_event );
      }
   }

   edbg_report_timed_events(ctx->dbgBuffer, ctx, &curr_time);
   edbg_report_fd_events(ctx->dbgBuffer, ctx);

   ipc_printf_buffer(ctx->dbgBuffer, "  \"current_time\": %ld.%06ld\n}",
                   curr_time.tv_sec, curr_time.tv_usec);

   // Send the data out to all clients
   zmql_broadcast_buffer(ctx->dbgServer, ctx->dbgBuffer);
}

void EVT_set_debugger_port(EVTHandler *handler, int port)
{
   handler->dbgPort = port;
}

void evt_fd_set_pausable(EVTHandler *ctx, int fd, char pausable)
{
   struct EventCB *curr;
   int event;

   for (curr = ctx->events[fd % ctx->hashSize]; curr; curr = curr->next) {
      if (curr->fd == fd) {
         curr->pausable = pausable;

         for (event = 0; event < EVENT_MAX; event++) {
            if (curr->cb[event] && (!curr->pausable || !curr->paused[event]) )
               FD_SET(fd, &ctx->blockedSet[event]);
            else
               FD_CLR(fd, &ctx->blockedSet[event]);
         }
      }
   }
}

void evt_fd_set_paused(EVTHandler *ctx, int fd, char paused)
{
   struct EventCB *curr;
   int event;

   for (curr = ctx->events[fd % ctx->hashSize]; curr; curr = curr->next) {
      if (curr->fd == fd) {
         for (event = 0; event < EVENT_MAX; event++) {
            curr->paused[event] = paused;
            if (curr->cb[event] && (!curr->pausable || !curr->paused[event]) )
               FD_SET(fd, &ctx->blockedSet[event]);
            else
               FD_CLR(fd, &ctx->blockedSet[event]);
         }
      }
   }
}
