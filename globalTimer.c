#include "eventTimer.h"
#include "debug.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

/*
 * Global Shared Timer support
 *
 * This timer uses shared memory and interprocess synchronization to 
 * maintain a single, gloabl shared virtual timer.  This is useful when running
 * tests / simulations that involve multiple processes that need strict global
 * ordering on their clocks.  Since this is a more sophisticated timer, it
 * is implemented in it's own file and has a bit more documentation involved.
 *
 * The synchronization is based on a shared memory region accessible via
 * a file in the filesystem.  The region is laid out as follows:
 *     1. Event Loop Mutex - A semaphore used to allow only one process to execute a non-debug event at a time.
 *     2. Time Barrier - A semaphore used as a barrier to inform processes when the global time has advanced and given them the opportunity to compete for the event loop mutex, as appropiate.
 *     3. Current Time - the current simulated global time
 *     4. Number of processes - the number of entries in the process array
 *     5. Process Info Array - An array of timer data, one for each process
 *          a.  Next time - The next virtual time the process needs to run
 *          b.  Active - A flag that marks active processes.  Used to recover from processes that don't exit the system cleanly
 *          c.  Mutex held - A boolean flag indicating if the process currently holds the event look mutex
 *          d.  process id
 *
 * A process must hold the event loop mutex prior to executing any global
 * time-dependent event loop activities.  This ensures only one time step is
 * taken at a time.  Additionally, the event loop mutex must be held to
 * update any of the data in the shared memory segment except to set the
 * active flag to 1.
 *
 * Initializing shared memory.
 * Shared memory is initialized by the first process entering the global
 * simulation.  To avoid race conditions with two processes entering at nearly
 * the same time, the posix advisory file locking mechanism (flock) is used.
 *
 * The per-process portion of the shared memory is initialized prior to
 * entering the event loop.  This requires obtaining the mutex, adding an entry
 * for the new process, synchronizing the new process's virtual clock with the
 * global clock, and advancing to the barrier.
 *
 * Barrier operation
 * The barrier is implemented with a semaphore.  Prior to releasing the mutex
 * the current process updates the global time by iterating through the shared
 * memory data structure to find the next smallest time.  It also calls up() on
 * the semaphore once for each active process, including itself.  It marks the
 * process with the smallest time as inactive to help cull processes that
 * may have crashed.  After releasing the mutex it waits at the barrier, which
 * should result in being immediately let through the barrier.
 *
 * Once through the barrier all processes in the system compete for the mutex.
 */

#define MAX_PROCS 128

struct SharedProcessState {
   struct timeval next_time;
   int active;
   int holds_mutex;
   int thief;
   pid_t pid;
};

struct SharedState {
   sem_t evt_mutex;
   sem_t barrier;
   struct timeval curr_time;
   int num_procs;
   int time_thief;
   struct SharedProcessState procs[MAX_PROCS];
};

struct VirtualGlobalEventTimer {
   struct EventTimer et;
   struct timeval time;
   struct SharedState *state;
   struct SharedProcessState *my_state;
   int state_fd;
   char *state_file;
   char paused;
};

char et_gvirt_get_pause(struct EventTimer *et);
void et_gvirt_set_pause(struct EventTimer *et, char pauseState);
int et_gvirt_get_time(struct EventTimer *et, struct timeval *time);
void et_gvirt_set_time(struct EventTimer *et, struct timeval *time);
void et_gvirt_inc_time(struct EventTimer *et, struct timeval *time);

static int setup_shared_state(struct VirtualGlobalEventTimer *self)
{
   int res;
   struct stat finfo;
   int init = 0;

   if (!self || !self->state_file)
      return -1;

   if ((res = open(self->state_file, O_RDWR | O_CREAT, 0666)) <= 0) {
      ERRNO_WARN("Failed to open global state file");
      return res;
   }
   self->state_fd = res;

   while (-1 == flock(self->state_fd, LOCK_EX)) {
      if (errno != EINTR) {
         ERRNO_WARN("Failed to flock global state file");
         close(self->state_fd);
         return -1;
      }
   }

   if (fstat(self->state_fd, &finfo) == -1) {
      ERRNO_WARN("Failed to stat global state file");
      close(self->state_fd);
      return -1;
   }

   // Initialize the file if too small
   if (finfo.st_size < sizeof(struct SharedState)) {
      init = 1;
      if (ftruncate(self->state_fd, sizeof(struct SharedState)) == -1) {
         ERRNO_WARN("Failed to resize global state file");
         close(self->state_fd);
         return -1;
      }
      if (fstat(self->state_fd, &finfo) == -1) {
         ERRNO_WARN("Failed to re-stat global state file");
         close(self->state_fd);
         close(self->state_fd);
         return -1;
      }
   }

   self->state = (struct SharedState*)mmap(NULL, finfo.st_size,
         PROT_READ | PROT_WRITE, MAP_SHARED, self->state_fd, 0);
   if (self->state == (void*)-1) {
      ERRNO_WARN("Failed to map timer shared memory");
      close(self->state_fd);
      return -1;
   }

   if (init) {
      if (-1 == sem_init(&self->state->evt_mutex, 1, 1)) {
         ERRNO_WARN("Failed to initialize event mutex");
         munmap(self->state, sizeof(*self->state));
         close(self->state_fd);
         return -1;
      }
      if (-1 == sem_init(&self->state->barrier, 1, 0)) {
         ERRNO_WARN("Failed to initialize barrier");
         munmap(self->state, sizeof(*self->state));
         close(self->state_fd);
         return -1;
      }
      self->state->num_procs = 0;
      gettimeofday(&self->state->curr_time, NULL);
   }

   if (-1 == sem_wait(&self->state->evt_mutex)) {
      ERRNO_WARN("Failed to get event mutex");
      munmap(self->state, sizeof(*self->state));
      close(self->state_fd);
      return -1;
   }

   self->my_state = &self->state->procs[self->state->num_procs++];
   self->my_state->next_time = self->state->curr_time;
   self->my_state->active = 1;
   self->my_state->holds_mutex = 1;
   self->my_state->pid = getpid();

   flock(self->state_fd, LOCK_UN);

   return 0;
}

static int cleanup_shared_state(struct VirtualGlobalEventTimer *self)
{
   int i, cnt = 0, del_file = 0;

   self->my_state->active = 0;

   // release the barrier if we hold the event mutex
   if (self->my_state->holds_mutex) {
      if (self->my_state->pid == self->state->time_thief &&
            self->state->time_thief)
         self->state->time_thief = 0;
      self->my_state->pid = 0;

      del_file = 1;
      for (i = 0; i < self->state->num_procs; i++) {
         if (self->state->procs[i].pid > 0 && self->state->procs[i].active) {
            sem_post(&self->state->barrier);
            cnt++;
         }
      }
      self->my_state->holds_mutex = 0;
      sem_post(&self->state->evt_mutex);
      flock(self->state_fd, LOCK_EX);
   }

   self->my_state->pid = 0;

   if (cnt == 0 && del_file) {
      sem_destroy(&self->state->evt_mutex);
      sem_destroy(&self->state->barrier);
   }

   munmap(self->state, sizeof(*self->state));
   self->state = NULL;
   close(self->state_fd);
   self->state_fd = 0;

   if (cnt == 0 && del_file) {
      flock(self->state_fd, LOCK_UN);
      unlink(self->state_file);
   }

   free(self->state_file);

   return 0;
}

int ET_gvirt_block(struct EventTimer *e, struct timeval *nextAwake,
      int pauseWhileBlocking, ET_block_cb blockcb, void *arg)
{
   struct timeval diffTime, curTime, *blockTime = NULL;
   struct VirtualGlobalEventTimer *et = (struct VirtualGlobalEventTimer *)e;
   
   // Set amount of time to block on select.
   // When time is "paused" we don't attempt to advance the virtual clock.
   //  This is the case for internal debugging events.
   if (nextAwake && pauseWhileBlocking) {
      et->et.get_monotonic_time(&et->et, &curTime);
      timersub(nextAwake, &curTime, &diffTime);
      if (diffTime.tv_sec < 0 || diffTime.tv_usec < 0) {
         memset(&diffTime, 0, sizeof(struct timeval));
      }
      blockTime = &diffTime;
   }
   // Block on select indefinetly if virtual clock is paused
   else if (nextAwake && (et->paused == VIRT_CLK_ACTIVE ||
            et->paused == VIRT_CLK_STOLEN) ) {
      assert(et->my_state->holds_mutex);
      // Finished with a trip through the global loop.
      // Update local time
      et->my_state->next_time = *nextAwake;

      // Compute next global time
      int smallest = -1;
      int proc_cnt = 0;
      for (int i = 0; i < et->state->num_procs; i++) {
         if (et->state->procs[i].pid > 0 && et->state->procs[i].active) {
            proc_cnt++;
            if (smallest < 0 || timercmp(&et->state->procs[i].next_time,
                     &et->state->procs[smallest].next_time, <))
               smallest = i;
         }
      }
      assert(smallest >= 0);

      if (!et->state->time_thief) {
         // Update global state with next time
         et->state->curr_time = et->state->procs[smallest].next_time;
         // Mark next process as inactive to detect crashes
         et->state->procs[smallest].active = 0;

         // Release the barrier
         for (int i = 0; i < proc_cnt; i++)
            sem_post(&et->state->barrier);
      }

      // Release the event mutex for competition
      et->my_state->thief = 0;
      if (et->state->time_thief && et->state->time_thief == et->my_state->pid)
         et->my_state->thief = 1;
      et->my_state->holds_mutex = 0;
      sem_post(&et->state->evt_mutex);

      // If we are stealing time then we skip the barrier and just block
      //  without updating the global clock's idea of time
      if (et->my_state->thief) {
         int res;

         et->et.get_monotonic_time(&et->et, &curTime);
         timersub(nextAwake, &curTime, &diffTime);
         if (diffTime.tv_sec < 0 || diffTime.tv_usec < 0) {
            memset(&diffTime, 0, sizeof(struct timeval));
         }
         blockTime = &diffTime;
         res = blockcb(&et->et, blockTime, arg);

         // re-lock the event mutex prior to returning to the event loop
         while (-1 == sem_wait(&et->state->evt_mutex))
            ;
         return res;
      }

      // Wait at the barrier
      while(1) {
         int res = sem_wait(&et->state->barrier);

         if (res < 0)
            continue;

         // Through the barrier.  Get the event loop mutex
         while (-1 == sem_wait(&et->state->evt_mutex))
            ;

         et_gvirt_set_time(&et->et, &et->state->curr_time);
         assert(timercmp(nextAwake, &et->my_state->next_time, ==));
         // Check to see if it is time to run through our loop
         if (timercmp(nextAwake, &et->state->curr_time, >)) {
            sem_post(&et->state->evt_mutex);
            continue;
         }
         et->my_state->active = 1;
         et->my_state->holds_mutex = 1;
         break;
      }

      // At this point we hold the event loop mutex and can take a turn through
      //  the loop

      memset(&diffTime, 0, sizeof(struct timeval));
      blockTime = &diffTime;
   }

   return blockcb(&et->et, blockTime, arg);
}

void ET_gvirt_cleanup(struct EventTimer *et)
{
   struct VirtualGlobalEventTimer *vet = (struct VirtualGlobalEventTimer *)et;
   
   if (vet) {
      cleanup_shared_state(vet);
      free(vet);
   }
}

struct EventTimer *ET_gvirt_init(const char *shared_state, char pause_mode)
{
   struct VirtualGlobalEventTimer *et;

   if (!shared_state)
      return NULL;

   et = malloc(sizeof(struct VirtualGlobalEventTimer));
   if (!et)
      return NULL;
   memset(et, 0, sizeof(struct VirtualGlobalEventTimer));

   et->state_file = strdup(shared_state);
   et->paused = pause_mode;
   et->et.block = &ET_gvirt_block;
   et->et.get_gmt_time = &et_gvirt_get_time;
   et->et.get_monotonic_time = &et_gvirt_get_time;
   et->et.cleanup = &ET_gvirt_cleanup;
   et->et.virt_inc_time = &et_gvirt_inc_time;
   et->et.virt_set_time = &et_gvirt_set_time;
   et->et.virt_get_time = &et_gvirt_get_time;
   et->et.virt_set_pause = &et_gvirt_set_pause;
   et->et.virt_get_pause = &et_gvirt_get_pause;

   if (setup_shared_state(et) < 0) {
      free(et->state_file);
      free(et);
      return NULL;
   }

   return (struct EventTimer *)et;
}

/**
 * Increment virtual clock time.
 *
 * @param clk a Virtual EventTimer object.
 * @param time to increment the clock by.
 */
void et_gvirt_inc_time(struct EventTimer *et, struct timeval *time)
{
   struct VirtualGlobalEventTimer *clk = (struct VirtualGlobalEventTimer *)et;

   assert(et);
   
   timeradd(time, &clk->time, &clk->time);
}

/**
 * Set virtual clock time.
 *
 * @param clk a Virtual EventTimer object.
 * @param time to set the clock to.
 */
void et_gvirt_set_time(struct EventTimer *et, struct timeval *time)
{
   struct VirtualGlobalEventTimer *clk = (struct VirtualGlobalEventTimer *)et;
   
   assert(et);

   clk->time = *time;
}

/**
 * Get virtual clock time. Advanced usage only. Most applications 
 * should use 'EVT_get_gmt_time' instead.
 *
 * @param clk a Virtual EventTimer object.
 */
int et_gvirt_get_time(struct EventTimer *et, struct timeval *time)
{
   struct VirtualGlobalEventTimer *clk = (struct VirtualGlobalEventTimer *)et;

   assert(et);

   *time = clk->time;
   return 0;
}

/**
 * Set Virtual EventTimer pause state
 *
 * @param clk a Virtual EventTimer object.
 * @param pauseState ET_virt_PAUSED or CLK_ACTIVE.
 */
void et_gvirt_set_pause(struct EventTimer *et, char pauseState)
{
   struct VirtualGlobalEventTimer *clk = (struct VirtualGlobalEventTimer *)et;

   assert(et);

   if (!clk->my_state->holds_mutex)
      return;

   if (pauseState == VIRT_CLK_STOLEN) {
      if (clk->state->time_thief)
         return;
      clk->state->time_thief = clk->my_state->pid;
   }
   else {
      if (clk->state->time_thief &&
            clk->state->time_thief == clk->my_state->pid)
         clk->state->time_thief = 0;
   }

   clk->paused = pauseState;
}

/**
 * Get Virtual EventTimer pause state
 *
 * @param clk a Virtual EventTimer object.
 *
 * @return VIRT_CLK_PAUSED or VIRT_CLK_ACTIVE.
 */
char et_gvirt_get_pause(struct EventTimer *et)
{
   struct VirtualGlobalEventTimer *clk = (struct VirtualGlobalEventTimer *)et;
   
   assert(et);

   return clk->paused;
}
