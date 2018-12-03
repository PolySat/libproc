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
 * @file events.h Handles events.
 *
 * @author Greg Manyak
 */
#ifndef EVENTS_H
#define EVENTS_H

#include <time.h>
#include <sys/select.h>

#include "priorityQueue.h"
#include "ipc.h"
#include "zmqlite.h"

#ifdef __cplusplus
extern "C" {
#endif

/// The file descriptor event types
#define EVENT_FD_READ  0 // Read FD event type
#define EVENT_FD_WRITE 1 // Write FD event type
#define EVENT_FD_ERROR 2 // Error FD event type
#define EVENT_MAX      3 // Number of event types

/// Defines to add or remove an event type
#define EVENT_KEEP     1 // Keep an event
#define EVENT_REMOVE   2 // Remove an event

typedef void (*EVT_debug_state_cb)(struct IPCBuffer*, void*);

enum EVTDebuggerState {
   /// The debugger is disabled
   EDBG_DISABLED = 1,
   /// The debuger is enabled
   EDBG_ENABLED = 2,
   /// The debugger is enabled and the process is stopped
   EDBG_STOPPED = 3,
};


// A callback for a file descriptor event
typedef int (*EVT_fd_cb)(int fd, char type, void *arg);

// A callback for a scheduled event
typedef int (*EVT_sched_cb)(void *arg);

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

struct EDBGClient;

// A structure which contains information regarding the state of the event handler
typedef struct EventState
{
   fd_set eventSet[EVENT_MAX];				                // File descriptor sets to watch
   int maxFd, maxFds[EVENT_MAX], eventCnt[EVENT_MAX];	// fd information
   int hashSize;				    	                        // The hash size of the event handler
   int keepGoing;				    	                        // Whether the handler should loop or not
   struct GPIOInterruptDesc gpio_intrs[2];            // GPIO interrupt state
   pqueue_t *queue;                                   // The schedule queue
   struct EventTimer *evt_timer;

   enum EVTDebuggerState initialDebuggerState;
   int dbgPort;
   struct ZMQLServer *dbgServer;
   struct IPCBuffer *dbgBuffer;
   unsigned long long loop_counter;
   int pause;
   EVT_debug_state_cb debuggerStateCB;
   void *debuggerStateArg;

   // MUST be last entry in struct
   EventCBPtr events[1];				                      // List of pointers to event callbacks
} EVTHandler;

/**
 * Create an event handler.
 * @param arg Context parameter passed to debugging related functions.
 *
 * @return The event handler.
 */
EVTHandler *EVT_create_handler(EVT_debug_state_cb debug_cb, void *arg);

/**
 * Free an event handler.
 *
 * @param handler The event handler.
 */
void EVT_free_handler(EVTHandler *handler);

/**
 * Add event callback for a file descriptor and event type.
 * Only one callback can be set for a file descript and event type combo.
 *
 * @param handler The event handler.
 * @param fd The file descriptor.
 * @param type Type of file descriptor event (EVENT_FD_READ, EVENT_FD_WRITE,
 * or EVENT_FD_ERROR).
 * @param cb The event callback.
 * @param arg The callback argument.
 *
 * @return 1 on success, 0 on failure.
 */
char EVT_fd_add(EVTHandler *handler, int fd, int type, EVT_fd_cb cb,
                  void *arg);

/**
 * Add event and cleanup callback for a file descriptor and event type.
 * Event callback behaves like described in EVT_fd_add.
 * The cleanup callback is automatically called once when the fd is removed
 * from the event loop.  A cleanup handler is required, but may be
 * useful in some applications.  When called, the cleanup handler
 * is passed the same void* arg that was used in EVT_fd_add.
 * Only one cleanup handler can be set for a file descript and event
 * type combo.
 *
 * @param handler The event handler.
 * @param fd The file descriptor.
 * @param type Type of file descriptor event (EVENT_FD_READ, EVENT_FD_WRITE,
 * or EVENT_FD_ERROR).
 * @param cb The event callback.
 * @param cleanup_cb The cleanup callback.
 * @param arg The callback argument.
 *
 * @return 1 on success, 0 on failure.
 */
char EVT_fd_add_with_cleanup(EVTHandler *handler, int fd, int type,
      EVT_fd_cb cb, EVT_fd_cb cleanup_cb, void *arg);

/**
 * Remove event callback for a file descriptor and event type.
 *
 * @param handler The event handler.
 * @param fd The file descriptor.
 * @param type Type of file descriptor event (EVENT_FD_READ, EVENT_FD_WRITE,
 * or EVENT_FD_ERROR).
 */
void EVT_fd_remove(EVTHandler *handler, int fd, int type);

/**
  * Convert a millisecond value into a timeval suitable for registering
  * events.
  *
  * @param ms milliseconds
  * @return The corresponding timeval structure
  */
struct timeval EVT_ms2tv(int ms);

/**
 * Add a scheduled event callback.
 *
 * @param handler The event handler.
 * @param time The time when the event should occur.
 * @param cb The event callback.
 * @param arg The callback argument.
 *
 * @return An unique identifier for the scheduled event.
 *   This identifier is necessary to change the event later.
 */
void *EVT_sched_add(EVTHandler *handler, struct timeval time,
      EVT_sched_cb cb, void *arg);

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
      struct timeval timestep, EVT_sched_cb cb, void *arg);

/**
 * Remove a scheduled event.
 *
 * @param handler The event handler.
 * @param event The event to remove.
 *
 * @return The arg provided when registering the event, or NULL if the
 *  event is invalid.
 */
void *EVT_sched_remove(EVTHandler *handler, void *eventId);

/**
 * Update a scheduled event.  The new full time will elapse before
 *   the callback is called.
 *
 * @param handler The event handler.
 * @param event The event to change.
 * @param time The time until the event should occur.
 *
 * @return 0 on success, other value if it is not found
 */
char EVT_sched_update(EVTHandler *handler, void *eventId, struct timeval time);

/**
 * Update a scheduled event. The event is given credit for the time
 *   it has already waited, potentially being called immediately.
 *
 * @param handler The event handler.
 * @param event The event to change.
 * @param time The time until the event should occur.
 *
 * @return 0 on success, other value if it is not found
 */
char EVT_sched_update_partial_credit(EVTHandler *handler, void *eventId, struct timeval time);

/**
 * Starts the main event loop.  Control of the program is given to the
 * event handler, which will block until an event occurs, and call the
 * appropriate callback.
 *
 * @param handler The event handler.
 *
 * @return 0 on an successfuly exit, -1 if an error occurs in the main loop.
 */
char EVT_start_loop(EVTHandler *handler);

/**
 * Ends the main event loop.
 *
 * @param handler The event handler.
 */
void EVT_exit_loop(EVTHandler *handler);

/**
 * Get the system's current absolute GMT time.
 *
 * @param tv A pointer to the timeval structure where the time gets stored
 *
 */
int EVT_get_gmt_time(EVTHandler *ctx, struct timeval *tv);

/**
 * USE ONLY WHEN ABSOLUTELY NECESSARY!
 *
 * Stateless version of EVT_get_gmt_time. Get the system's current 
 * absolute GMT time without passing a EvtHander context. Use 
 * only when absolutely necessary!
 *
 * @param tv A pointer to the timeval structure where the time gets stored
 */
int EVT_get_gmt_time_virt(struct timeval *tv);

/**
 * Get the system's current time since an unknown reference point.  This
 * increases monotonically despite any changes to the system's current
 * understanding of GMT.
 *
 * @param tv A pointer to the timeval structure where the time gets stored
 */
int EVT_get_monotonic_time(EVTHandler *ctx, struct timeval *tv);

/**
 * Enable libproc virtual clock. Sets EventTimer to a new
 * instance of a VirtualEventTimer.
 *
 * @param ctx  EVTHandler struct
 * @param tv   A pointer to the timeval with the desired initial time.
 */
char EVT_enable_virt(EVTHandler *ctx, struct timeval *initTime);

/**
 * Get current libproc EventTimer.
 *
 * @param ctx  EVTHandler struct
 * @param et   The EventTimer instance.
 */
struct EventTimer *EVT_get_evt_timer(EVTHandler *ctx);

/**
 * Set libproc EventTimer.
 *
 * @param ctx  EVTHandler struct
 * @param et   The EventTimer instance.
 */
void EVT_set_evt_timer(EVTHandler *ctx, struct EventTimer *et);

/**
 * Subracts one timeval struct from another and stores the result.
 *
 * @param x The minuend
 * @param y The subtrahend
 * @param result A pointer to where to store the result
 * @return 1 if the difference is negative, otherwise 0
 */
int timeval_subtract(struct timeval *result, struct timeval *x,
	struct timeval *y);

/** Registers for a callback upon deployment from the PPod
  * @param handler The event handler.
  * @param cb The callback
  * @param arg The parameter to pass into the callback
  * @return 0 if registration successful, negative otherwise
  */
int EVT_add_ppod_deployment_cb(EVTHandler *handler ,EVT_sched_cb cb, void *arg);

/** Registers for a callback upon deployment from the PPod
  * @param handler The event handler.
  * @param cb The callback
  * @param arg The parameter to pass into the callback
  * @return 0 if unregistration successful, negative otherwise
  */
int EVT_remove_ppod_deployment_cb(EVTHandler *handler, EVT_sched_cb cb, void *arg);

/** Registers for a callback when hardware warns of a reboot
  * @param handler The event handler.
  * @param cb The callback
  * @param arg The parameter to pass into the callback
  * @return 0 if registration successful, negative otherwise
  */
int EVT_add_pending_reboot_cb(EVTHandler *handler ,EVT_sched_cb cb, void *arg);

/** Unregisters for a callback when hardware warns of a reboot
  * @param handler The event handler.
  * @param cb The callback
  * @param arg The parameter to pass into the callback
  * @return 0 if unregistration successful, negative otherwise
  */
int EVT_remove_pending_reboot_cb(EVTHandler *handler, EVT_sched_cb cb, void *arg);

/** Sets the debugger's initial state to the provided paramater.  Can be
  * used to programatically enable remote event debugging.  This must be called
  * before entering the event loop, otherwise it has no effect.
  * @param handler The event handler.
  * @param st The state to use when initializing the debugger.
  */
void EVT_set_initial_debugger_state(EVTHandler *handler,
      enum EVTDebuggerState st);

/** Sets the TCP port used by the event debugger.
  *
  * @param handler The event handler.
  * @param port The port number to use
  */
void EVT_set_debugger_port(EVTHandler *handler, int port);

#ifdef __cplusplus
}

class EventManager
{
   public:
      EventManager() : free_state(true) {
         ctx = EVT_create_handler();
      }

      EventManager(struct EventState *state) : ctx(state),
            free_state(false) {}
      virtual ~EventManager() { if (free_state) EVT_free_handler(ctx); }

      int EventLoop() { return EVT_start_loop(ctx); }
      void Exit() { EVT_exit_loop(ctx); }

      void RemoveEvent(int fd, int event) { EVT_fd_remove(ctx, fd, event); }
      void RemoveReadEvent(int fd) { RemoveEvent(fd, EVENT_FD_READ); }
      void RemoveWriteEvent(int fd) { RemoveEvent(fd, EVENT_FD_WRITE); }
      void RemoveErrorEvent(int fd) { RemoveEvent(fd, EVENT_FD_ERROR); }

      struct EventState *state() { return ctx; }

      void AddEvent(int fd, EVT_fd_cb cb, void *p, int event,
            EVT_fd_cb cleanup = nullptr)
         { EVT_fd_add_with_cleanup(ctx, fd, event, cb, cleanup, p); }
      void AddReadEvent(int fd, EVT_fd_cb cb, void *p,
            EVT_fd_cb cleanup = nullptr)
         { AddEvent(fd, cb, p, EVENT_FD_READ, cleanup); }
      void AddWriteEvent(int fd, EVT_fd_cb cb, void *p,
            EVT_fd_cb cleanup = nullptr)
         { AddEvent(fd, cb, p, EVENT_FD_WRITE, cleanup); }
      void AddErrorEvent(int fd, EVT_fd_cb cb, void *p,
            EVT_fd_cb cleanup = nullptr)
         { AddEvent(fd, cb, p, EVENT_FD_ERROR, cleanup); }

      template<class T>
         void AddEvent(int fd, int (T::*cb)(int), T *p, int event) {
            void *arg = new WrapperClass<T>(cb, p);
            AddEvent(fd, &WrapperClass<T>::WrapperCB, arg, event,
                  &WrapperClass<T>::Cleanup);
         }
      template<class T> void AddReadEvent(int fd, int (T::*cb)(int), T *p)
         { AddEvent<T>(fd, cb, p, EVENT_FD_READ); }
      template<class T> void AddWriteEvent(int fd, int (T::*cb)(int), T *p)
         { AddEvent<T>(fd, cb, p, EVENT_FD_WRITE); }
      template<class T> void AddErrorEvent(int fd, int (T::*cb)(int), T *p)
         { AddEvent<T>(fd, cb, p, EVENT_FD_ERROR); }


   protected:
      EventManager(const EventManager&);
      const EventManager& operator=(const EventManager&);

      struct EventState *ctx;
      bool free_state;

      template<class T>
         class WrapperClass {
            protected:
               int (T::*cb)(int fd);
               T *me;

            public:
               WrapperClass( int (T::*c)(int fd), T *p) : cb(c), me(p) {}
               static int WrapperCB(int fd, char type, void *p) {
                  if (!p)
                     return EVENT_KEEP;

                  WrapperClass<T> *wrap = (WrapperClass<T>*)p;
                  return (wrap->me->*wrap->cb)(fd);
               }

               static int Cleanup(int fd, char type, void *p) {
                  if (!p)
                     return EVENT_KEEP;

                  WrapperClass<T> *wrap = (WrapperClass<T>*)p;
                  delete wrap;
                  return EVENT_KEEP;
               }
         };

};

#endif

#endif
