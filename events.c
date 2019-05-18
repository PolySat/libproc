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
// #include <polysat_pkt/watchdog_cmd.h>

// Static global for virtual time
static EVTHandler *global_evt = NULL;


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

// Structure representing a schedule callback
typedef struct _ScheduleCB
{
	struct timeval scheduleTime;
	struct timeval nextAwake;
	EVT_sched_cb callback;
	void *arg;
	size_t pos;
	struct timeval timeStep;
} ScheduleCB;

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

// Structure which defines a file callback
typedef struct EventCB
{
   EVT_fd_cb cb[EVENT_MAX];		  // An array of function callbacks to call
   EVT_fd_cb cleanup[EVENT_MAX];	// An array of cleanup callback to call
   void *arg[EVENT_MAX];			      // An array of arguments to pass to callbacks
   int fd;				                // The file descriptor which will launch the event
   struct EventCB *next;		      // The next signal callback
} *EventCBPtr;

struct GPIOInterruptCBList {
   EVT_sched_cb cb;
   void *arg;
   struct GPIOInterruptCBList *next;
};

struct GPIOInterruptDesc {
   const char *filename;
   int fd, tripped;
   struct GPIOInterruptCBList *callbacks;
};

// A structure which contains information regarding the state of the event handler
struct EventState
{
   fd_set eventSet[EVENT_MAX];				                // File descriptor sets to watch
   int maxFd, maxFds[EVENT_MAX], eventCnt[EVENT_MAX];	// fd information
   int hashSize;				    	                        // The hash size of the event handler
   int keepGoing;				    	                        // Whether the handler should loop or not
   struct GPIOInterruptDesc gpio_intrs[2];            // GPIO interrupt state
   pqueue_t *queue;                                   // The schedule queue
   struct EventTimer *evt_timer;

   EventCBPtr events[1];				                      // List of pointers to event callbacks
};

/* Initializes an EventState with a given hash size.
 * @param hashSize The hash size of the event handler.
 * @return A pointer to the new EventState
 */
struct EventState *EVT_initWithSize(int hashSize)
{
   struct EventState *res = NULL;
   int i;

   res = (struct EventState*)malloc(
         sizeof(struct EventState) + hashSize * sizeof(EventCBPtr));
   if (!res){
      return NULL;
   }

   memset(&res->gpio_intrs, 0, sizeof(res->gpio_intrs));

   res->keepGoing = 1;
   for ( i = 0; i < EVENT_MAX; i++) {
      FD_ZERO(&res->eventSet[i]);
      res->maxFds[i] = 0;
      res->eventCnt[i] = 0;
   }
   res->hashSize = hashSize;
   res->maxFd = 0;
   for (i = 0; i < res->hashSize; i++)
      res->events[i] = NULL;

   res->queue = pqueue_init(hashSize, cmp_pri, get_pri, set_pri, get_pos, set_pos);
   if (res->queue == NULL){
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
   return res;
}

/* Initializes an EventState with a hash size of 19
 * @return A pointer to the new EventState
 */
EVTHandler *EVT_create_handler()
{
   return (EVTHandler *)EVT_initWithSize(19);
}

struct timeval EVT_sched_remaining(EVTHandler *handler, void *eventId)
{
   ScheduleCB *evt = (ScheduleCB*)eventId;
   struct timeval now, result;

   handler->evt_timer->get_monotonic_time(handler->evt_timer, &now);
   timersub(&evt->nextAwake, &now, &result);

   return result;
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

	if (tmp->fd == ctx->maxFds[event]) {
      ctx->maxFds[event] = 0;
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

   if (ctx->evt_timer)
      ctx->evt_timer->cleanup(ctx->evt_timer);
   global_evt = NULL;

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
       free(curProc);
   }

   pqueue_free(ctx->queue);
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
      }
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

struct EVT_select_cb_args {
   fd_set *eventSetPtrs[EVENT_MAX];
   int maxFd;
};

static int select_event_loop_cb(struct EventTimer *et,
    struct timeval *nextAwake, void *opaque)
{
   struct EVT_select_cb_args *args = (struct EVT_select_cb_args*)opaque;

   return select(args->maxFd, args->eventSetPtrs[EVENT_FD_READ],
                  args->eventSetPtrs[EVENT_FD_WRITE],
                  args->eventSetPtrs[EVENT_FD_ERROR], nextAwake);
}

char EVT_start_loop(EVTHandler *ctx)
{
   fd_set eventSets[EVENT_MAX];
   struct EVT_select_cb_args args;
   int i;
   int retval;
   int event, fd;
   struct EventCB **evtCurr;
   int keep;
   int startEvent = EVENT_FD_READ;
   int startFd = 0;
   struct timeval curTime, *nextAwake;
   ScheduleCB *curProc;

   while(ctx->keepGoing) {
      for (i = 0; i < EVENT_MAX; i++) {
         if (ctx->eventCnt[i] > 0) {
            args.eventSetPtrs[i] = &eventSets[i];
            //FD_COPY(&ctx->eventSet[i], args.eventSetPtrs[i]);
				memcpy(args.eventSetPtrs[i], &ctx->eventSet[i], sizeof(*(&ctx->eventSet[i])));
         }
         else
            args.eventSetPtrs[i] = NULL;
      }

      args.maxFd = ctx->maxFd + 1;

      curProc = pqueue_peek(ctx->queue);
      if (curProc)
         nextAwake = &curProc->nextAwake;
      else
         nextAwake = NULL;
      
      // Call blocking function of event timer
      retval = ctx->evt_timer->block(ctx->evt_timer, nextAwake,
                     &select_event_loop_cb, &args);

      // Process Timed Events
      while ((curProc = pqueue_peek(ctx->queue))) {
         ctx->evt_timer->get_monotonic_time(ctx->evt_timer, &curTime);

         if (timercmp(&curProc->nextAwake, &curTime, >)) {
            // Event is not yet ready
            break;
         }

         // Pop event from the queue
         pqueue_pop(ctx->queue);

         // Call the callback and see if it wants to be kept
         if (curProc->callback(curProc->arg) == EVENT_KEEP) {
            curProc->scheduleTime = curTime;
            timeradd(&curProc->nextAwake,
               &curProc->timeStep, &curProc->nextAwake);
            pqueue_insert(ctx->queue, curProc);
         } else {
            free(curProc);
         }
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
                     keep = EVENT_KEEP;
                     for(evtCurr = &ctx->events[fd % ctx->hashSize];
                           *evtCurr; evtCurr = &(*evtCurr)->next) {
                        if ((*evtCurr)->fd != fd)
                           continue;
                        if ((*evtCurr)->cb[event]) {
                           keep = (*(*evtCurr)->cb[event])
                              (fd, event, (*evtCurr)->arg[event]);
                        }
                        break;
                     }

                     if (EVENT_REMOVE == keep){
                        EVT_remove_internal(ctx, evtCurr, event);
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
         if (errno == 0 || errno == EINTR){
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

   if (0 == pqueue_insert(handler->queue, newSchedCB)){
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

   if (0 == pqueue_insert(handler->queue, newSchedCB)){
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
   void *result = NULL;

   if (0 == pqueue_remove(handler->queue, eventId)) {
      result = ((ScheduleCB*)eventId)->arg;
      free(eventId);
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

   handler->evt_timer->get_monotonic_time(handler->evt_timer, &evt->scheduleTime);
   timeradd(&evt->scheduleTime, &time, &evt->nextAwake);
   evt->timeStep = time;
   pqueue_change_priority(handler->queue, evt->nextAwake, evt);

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

   handler->evt_timer->get_monotonic_time(handler->evt_timer, &now);
   if (timercmp(&evt->nextAwake, &now, <=))
      evt->nextAwake = now;

   printf("%lu\n", evt->nextAwake.tv_sec);
   evt->timeStep = time;
   pqueue_change_priority(handler->queue, evt->nextAwake, evt);

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
