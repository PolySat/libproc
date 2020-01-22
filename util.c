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
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>

#include "util.h"
#include "ipc.h"
#include "priorityQueue.h"
#include "debug.h"

static const char *serialNumFileName = "/proc/cpuinfo";

unsigned long UTIL_board_sn(void)
{
   static unsigned long serialNum = ULONG_MAX;
   FILE *in;
   char inb[4096];
   char *delm, *value, *label;
   int end;

   if (serialNum != ULONG_MAX)
      return serialNum;

   in = fopen(serialNumFileName, "r");
   if (!in) {
      perror("Error opening serial input data file");
      return 0;
   }

   while (NULL != fgets(inb, sizeof(inb), in)) {
      if (!(delm = strchr(inb, ':')))
         continue;

      for (value = delm + 1; *value && isspace(*value); value++)
         ;
      end = strlen(value) - 1;
      while(end >= 0 && isspace(value[end]))
         value[end--] = 0;

      for (label = inb; label < delm && isspace(*label); label++)
         ;
      *delm = 0;
      end = strlen(label) - 1;
      while(end >= 0 && isspace(label[end]))
         label[end--] = 0;

      if (0 == strcmp("Serial", label)) {
         serialNum = strtol(value, NULL, 16);
         break;
      }
   }

   fclose(in);

   return serialNum;
}

int UTIL_board_type(void)
{
   static int boardType = BOARD_TYPE_UNKNOWN;
   FILE *in;
   char inb[4096];
   char *delm, *value, *label;
   int end;

   if (boardType != BOARD_TYPE_UNKNOWN)
      return boardType;

   in = fopen(serialNumFileName, "r");
   if (!in) {
      perror("Error opening board type input data file");
      return 0;
   }

   while (NULL != fgets(inb, sizeof(inb), in)) {
      if (!(delm = strchr(inb, ':')))
         continue;

      for (value = delm + 1; *value && isspace(*value); value++)
         ;
      end = strlen(value) - 1;
      while(end >= 0 && isspace(value[end]))
         value[end--] = 0;

      for (label = inb; label < delm && isspace(*label); label++)
         ;
      *delm = 0;
      end = strlen(label) - 1;
      while(end >= 0 && isspace(label[end]))
         label[end--] = 0;

      if (0 == strcmp("Hardware", label)) {
         if (0 == strcmp("PolySat Avionics Board, version 1", value))
            boardType = BOARD_TYPE_INTREPID;
         else if (0 == strcmp("Atmel AT91SAM9G20-EK 2 MMC Slot Mod", value))
            boardType = BOARD_TYPE_DEV;
         break;
      }
   }

   fclose(in);

   return boardType;
}

struct CleanupState
{
   struct dirent dirent;
   struct timeval score;
   size_t pos;
};

struct UnlinkNode {
   char *file;
   struct UnlinkNode *next;
};

static struct timeval get_pri_cleanup_state(void *a)
{
   return ((struct CleanupState *) a)->score;
}

static void set_pri_cleanup_state(void *a, struct timeval pri)
{
   ((struct CleanupState *) a)->score = pri;
}

static int cmp_pri_cleanup_state(struct timeval next, struct timeval curr)
{
   return timercmp(&next, &curr, >=);
}

static size_t get_pos_cleanup_state(void *a)
{
   // printf("getpos %p: %lu\n", a, (unsigned long)((struct CleanupState *) a)->pos);
   return ((struct CleanupState *) a)->pos;
}

// Set position callback
static void set_pos_cleanup_state(void *a, size_t pos)
{
   // printf("setpos %p ->%lu\n", a, (unsigned long)pos);
   ((struct CleanupState *) a)->pos = pos;
}

int UTIL_cleanup_dir(const char *dirname, int max_entries, int ignore_hidden,
    cleanup_mod_time_cb_t modtime_cb, void *cb_arg, post_delete_cb_t del_cb,
    void *del_cb_arg)
{
   DIR *dir = NULL;
   struct dirent *prev_ent = NULL;
   ps_pqueue_t *queue = NULL;
   struct CleanupState *cleanup_records = NULL, *next_rec = NULL;
   int res = 0;
   char *path_buff = NULL;
   struct stat statbuff;
   struct UnlinkNode *unlink_list = NULL, *unlink_curs;

   if (!dirname)
      goto cleanup;

   if (max_entries <= 0)
      max_entries = 1;

   path_buff = malloc(sizeof(prev_ent->d_name) + strlen(dirname) + 10);
   if (!path_buff) {
      ERR_REPORT(DBG_LEVEL_WARN, "failed to allocate path_buff\n");
      res = -ENOMEM;
      goto cleanup;
   }
   // Pre-copy the directory path portion of the path buffer
   strcpy(path_buff, dirname);
   int name_offset = strlen(path_buff);
   // Pre-append the trailing /
   if ('/' != path_buff[name_offset - 1])
      path_buff[name_offset++] = '/';
   path_buff[name_offset] = 0;

   // Pre-allocate a pqueue (efficient data structure for keeping entries
   //  sorted) to hold max + 5 entries
   queue = ps_pqueue_init(max_entries + 5, cmp_pri_cleanup_state,
      get_pri_cleanup_state, set_pri_cleanup_state,
      get_pos_cleanup_state, set_pos_cleanup_state);

   if (!queue) {
      ERR_REPORT(DBG_LEVEL_WARN, "failed to allocate pqueue\n");
      res = -ENOMEM;
      goto cleanup;
   }

   // Pre-allocate max + 1 records to store in pqueue
   cleanup_records = malloc( (max_entries + 1) * sizeof(*cleanup_records));
   if (!queue) {
      ERR_REPORT(DBG_LEVEL_WARN, "failed to allocate pqueue\n");
      res = -ENOMEM;
      goto cleanup;
   }

   // Next record to use in pqueue.  if queue->size < max, use the next slot
   //  in pre-allocated array.  Otherwise use the record we just removed from
   //  queue.
   next_rec = cleanup_records;

   dir = opendir(dirname);
   if (!dir) {
      ERR_REPORT(DBG_LEVEL_WARN, "failed to open %s: %s\n", dirname,
                                 strerror(errno));
      res = -errno;
      goto cleanup;
   }


    // TODO: readdir_r is deprecated because readdir is generally threadsafe.
    // However, we haven't confirmed that ulibc's readdir is threadsafe,
    // so we'll will stick with it for now.
   while (0 == (res = readdir_r(dir, &next_rec->dirent, &prev_ent))
              && prev_ent) {
      if (0 == strcmp(".", next_rec->dirent.d_name))
         continue;
      if (0 == strcmp("..", next_rec->dirent.d_name))
         continue;
      if (ignore_hidden && next_rec->dirent.d_name[0] == '.')
         continue;

      // Stat the file to get mod time
      strcpy(&path_buff[name_offset], next_rec->dirent.d_name);
      // printf("%s\n", path_buff);
      if (0 != stat(path_buff, &statbuff)) {
         ERR_REPORT(DBG_LEVEL_WARN, "failed to stat %s: %s\n",
            path_buff, strerror(errno));
         continue;
      }
      if (modtime_cb)
         next_rec->score = (*modtime_cb)(&next_rec->dirent, path_buff,
            &statbuff, cb_arg);
      else {
         next_rec->score.tv_sec = statbuff.st_mtime;
         next_rec->score.tv_usec = 0;
      }

      // Add the file into our queue
      ps_pqueue_insert(queue, next_rec);

      // If the queue is too big (> max_entries), dequeue oldest entry,
      //  delete it, and reuse entry storage for next iteration through
      //  the readdir() loop
      if (ps_pqueue_size(queue) > max_entries) {
         next_rec = ps_pqueue_pop(queue);
         if (!next_rec) {
            ERR_REPORT(DBG_LEVEL_WARN, "pqueue internal error.  pop on non-empty queue didn't return record\n");
            res = -1;
            goto cleanup;
         }

         // Delete our new-found oldest file
         strcpy(&path_buff[name_offset], next_rec->dirent.d_name);
         unlink_curs = malloc(sizeof(*unlink_curs));
         if (unlink_curs) {
            unlink_curs->next = unlink_list;
            unlink_curs->file = strdup(path_buff);
            unlink_list = unlink_curs;
         }
#if 0
#endif
      }
      else {
         // We haven't filled the queue yet.  Advance next_rec to the
         //  next element in the preallocated array.  This is safe until
         //  we use all the elements, in which case the else should never
         //  happen
         next_rec++;
      }
   }

   for (unlink_curs = unlink_list; unlink_curs;
        unlink_curs = unlink_curs->next) {
      if (unlink_curs->file) {
         // printf("unlink %s\n", unlink_curs->file);
         if (0 != unlink(unlink_curs->file))
            ERR_REPORT(DBG_LEVEL_WARN, "failed to delete %s: %s\n",
               unlink_curs->file, strerror(errno));
         else if (del_cb)
            (*del_cb)(unlink_curs->file, del_cb_arg);
      }
   }

cleanup:
   while ((unlink_curs = unlink_list)) {
      unlink_list = unlink_curs->next;
      if (unlink_curs->file)
         free(unlink_curs->file);
      free(unlink_curs);
   }

   if (dir)
      closedir(dir);

   if (queue)
      ps_pqueue_free(queue);

   if (cleanup_records)
      free(cleanup_records);

   if (path_buff)
      free(path_buff);

   return res;
}

int UTIL_ensure_dir(const char *toDir)
{
   struct stat buff;

   if (stat(toDir, &buff)) {
      ERR_REPORT(DBG_LEVEL_WARN, "error stating", toDir);
      return 0;
   }

   if (!S_ISDIR(buff.st_mode)) {
      DBG_print(DBG_LEVEL_WARN, "%s is not a dir!", toDir);
      return 0;
   }

   return 1;
}

int UTIL_ensure_path(const char *toDir)
{
   char buff[PATH_MAX];

   if (UTIL_ensure_dir(toDir))
      return 1;

   snprintf(buff, sizeof(buff), "mkdir -p \"%s\"", toDir);
   buff[sizeof(buff)-1] = 0;

   if(system(buff) == -1) {
      ERR_REPORT(DBG_LEVEL_WARN, "error running mkdir");
      return 0;
   }

   return UTIL_ensure_dir(toDir);
}
