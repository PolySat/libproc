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
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "proclib.h"
#include "debug.h"
#include "critical.h"
#include "util.h"

#define CS_FILE_DIRECTORY "/critical_state"
#define CS_FILE_PREFIX "crit-state"
#define NUM_COPIES 4
#define CLEANUP_INTERVAL 6

struct CriticalEntry {
   uint32_t seqNumHigh;
   uint32_t seqNumLow;
   uint8_t reserved[8];
   uint8_t state[CRITICAL_STATE_MAX_LEN];
   uint8_t md5[MD5_DIGEST_LENGTH];
} __attribute__((packed));

struct CleanupNode {
   char *file;
   struct CleanupNode *next;
};

extern struct CSState *proc_get_cs_state(ProcessData *proc);

static void cleanup_old_state(struct CSFileState *fs, const char *proc_name)
{
   char full_path[PATH_MAX];
   char prefix[PATH_MAX];
   DIR *dir = NULL;
   struct dirent *ent = NULL;
   struct CleanupNode *curr, *head = NULL;

   if (!fs->curr_file)
      return;

   sprintf(prefix, "%s-%s.%s.", CS_FILE_PREFIX, proc_name, fs->prefix);

   dir = opendir(fs->directory);
   if (!dir) {
      ERR_REPORT(DBG_LEVEL_WARN, "failed to opendir %s: %s\n",
               fs->directory, strerror(errno));
      return;
   }

   while ((ent = readdir(dir))) {
      if (!ent->d_name[0] || ent->d_name[0] == '.')
         continue;
      if (strncmp(prefix, ent->d_name, strlen(prefix)))
         continue;
      sprintf(full_path, "%s/%s", fs->directory, ent->d_name);
      if (fs->curr_file && !strcmp(full_path, fs->curr_file))
         continue;

      curr = malloc(sizeof(*curr));
      curr->next = head;
      curr->file = strdup(full_path);
      head = curr;
   }

   closedir(dir);

   while((curr = head)) {
      head = curr->next;
      if (curr->file) {
         unlink(curr->file);
         free(curr->file);
      }
      free(curr);
   }
   fs->generation = 0;
}

static int write_cs_file(struct CSFileState *fs, const char *proc_name,
   struct CriticalEntry *ent)
{
   char file_name[PATH_MAX];
   int fd;
   int i;

   sprintf(file_name, "%s/%s-%s.%s.XXXXXX", fs->directory, CS_FILE_PREFIX,
          proc_name, fs->prefix);

   fd = mkstemp(file_name);
   if (fd < 0) {
      ERR_REPORT(DBG_LEVEL_WARN, "failed to create CS file %s: %s\n",
                        file_name, strerror(errno));
      return -1;
   }

   for (i = 0; i < NUM_COPIES; i++) {
      if (sizeof(*ent) != write(fd, ent, sizeof(*ent))) {
         ERR_REPORT(DBG_LEVEL_WARN, "bad/short write to CS file %s: %s\n",
                           file_name, strerror(errno));
         close(fd);
         unlink(file_name);
         return -2;
      }
   }

   close(fd);

   if (fs->curr_file)
      free(fs->curr_file);
   fs->curr_file = strdup(file_name);

   if (++fs->generation > CLEANUP_INTERVAL)
      cleanup_old_state(fs, proc_name);

   return 0;
}

static int process_critical_entry(struct CSState *cs, struct CriticalEntry *ent)
{
   MD5_CTX md5Ctx;
   unsigned char md5[MD5_DIGEST_LENGTH];
   uint64_t seq;

   MD5Init(&md5Ctx);
   MD5Update(&md5Ctx, (unsigned char*) ent, sizeof(*ent) - sizeof(ent->md5));
   MD5Final(md5, &md5Ctx);

   if (memcmp(ent->md5, md5, sizeof(md5)))
      return 0;

   seq = ntohl(ent->seqNumHigh);
   seq <<= 32;
   seq |= (uint32_t)ntohl(ent->seqNumLow);

   if (seq < cs->state_version)
      return 0;

   if (seq == cs->state_version)
      return 1;

   cs->state_version = seq;
   memcpy(cs->state, ent->state, sizeof(cs->state));
   MD5Init(&md5Ctx);
   MD5Update(&md5Ctx, (unsigned char*) cs->state, sizeof(cs->state));
   MD5Final(cs->md5, &md5Ctx);

   return 1;
}

static int load_critical_state_file(struct CSState *cs, const char *file)
{
   int fd;
   int len;
   struct CriticalEntry ent;
   int res = 0;

   if ((fd = open(file, O_RDONLY)) < 0) {
      ERR_REPORT(DBG_LEVEL_WARN, "failed to open %s: %s\n",
               file, strerror(errno));
      return -1;
   }

   while ((len = read(fd, &ent, sizeof(ent))) > 0) {
      if (len != sizeof(ent)) {
         ERR_REPORT(DBG_LEVEL_WARN, "short read %s\n", file);
         len = 0;
         break;
      }

      if (process_critical_entry(cs, &ent))
         res = 1;
   }

   if (len < 0) {
      ERR_REPORT(DBG_LEVEL_WARN, "failed to read %s: %s\n",
               file, strerror(errno));
      res = -errno;
   }

   close(fd);

   return res;
}

static int load_critical_state_directory(struct CSState *cs,
   struct CSFileState *fs)
{
   char full_path[PATH_MAX];
   char prefix[PATH_MAX];
   DIR *dir = NULL;
   struct dirent *ent = NULL;
   int rd_cnt = 0;

   sprintf(prefix, "%s-%s.%s.", CS_FILE_PREFIX, cs->name, fs->prefix);

   dir = opendir(fs->directory);
   if (!dir) {
      ERR_REPORT(DBG_LEVEL_WARN, "failed to opendir %s: %s\n",
               fs->directory, strerror(errno));
      return -1;
   }

   while ((ent = readdir(dir))) {
      if (!ent->d_name[0] || ent->d_name[0] == '.')
         continue;
      if (strncmp(prefix, ent->d_name, strlen(prefix)))
         continue;

      sprintf(full_path, "%s/%s", fs->directory, ent->d_name);
      if (load_critical_state_file(cs, full_path) > 0) {
         rd_cnt++;
         if (fs->curr_file)
            free(fs->curr_file);
         fs->curr_file = strdup(full_path);
      }
   }

   closedir(dir);

   return rd_cnt;
}

static int load_critical_state(struct CSState *cs)
{
   int i;

   cs->state_version = 0;
   for (i = 0; i < CRITICAL_STATE_NUM_FILES; i++)
      load_critical_state_directory(cs, &cs->files[i]);

   cs->dirty = 0;

   return 0;
}

void critical_state_init(struct CSState *cs, const char *name)
{
   int i;

   memset(cs, 0, sizeof(*cs));
   cs->name = name;

   for (i = 0; i < CRITICAL_STATE_NUM_FILES; i++) {
      sprintf(cs->files[i].prefix, "%c", 'a' + i);
      cs->files[i].directory = strdup(CS_FILE_DIRECTORY);
   }

   UTIL_ensure_dir(CS_FILE_DIRECTORY);

   load_critical_state(cs);

   for (i = 0; i < CRITICAL_STATE_NUM_FILES; i++)
      cleanup_old_state(&cs->files[i], name);
}

void critical_state_cleanup(struct CSState *cs)
{
   int i;

   if (!cs)
      return;

   for (i = 0; i < CRITICAL_STATE_NUM_FILES; i++)
      cleanup_old_state(&cs->files[i], cs->name);

   for (i = 0; i < CRITICAL_STATE_NUM_FILES; i++) {
      if (cs->files[i].curr_file)
         free(cs->files[i].curr_file);
      cs->files[i].curr_file = NULL;
      if (cs->files[i].directory)
         free(cs->files[i].directory);
      cs->files[i].directory = NULL;
   }
}

int PROC_save_critical_state(ProcessData *proc, void *state, int len)
{
   struct CriticalEntry ent;
   int i;
   MD5_CTX md5;
   struct CSState *cs;

   if (len > sizeof(ent.state) || len <= 0)
      return -1;

   if (!proc)
      return -2;

   cs = proc_get_cs_state(proc);
   if (!cs)
      return -10;

   memset(&ent, 0, sizeof(ent));
   memcpy(&ent.state, state, len);

   cs->state_version++;
   ent.seqNumHigh = htonl( (cs->state_version >> 32) & 0xFFFFFFFF);
   ent.seqNumLow = htonl(cs->state_version & 0xFFFFFFFF);

   MD5Init(&md5);
   MD5Update(&md5, (unsigned char*) &ent, sizeof(ent) - sizeof(ent.md5));
   MD5Final(ent.md5, &md5);

   for (i = 0; i < CRITICAL_STATE_NUM_FILES; i++) {
      if (write_cs_file(&cs->files[i], cs->name, &ent) < 0) {
         if (i == 0)
            return -3;
         else {
            cs->dirty = 1;
            continue;
         }
      }
   }

   memcpy(cs->state, &ent.state, sizeof(ent.state));
   MD5Init(&md5);
   MD5Update(&md5, (unsigned char*) cs->state, sizeof(cs->state));
   MD5Final(cs->md5, &md5);

   return len;
}

int PROC_read_critical_state(ProcessData *proc, void *state, int len)
{
   struct CSState *cs = proc_get_cs_state(proc);
   MD5_CTX md5Ctx;
   unsigned char md5[MD5_DIGEST_LENGTH];

   if (!cs)
      return -10;

   // Load state if it is marked as dirty
   if (cs->dirty) {
      if (load_critical_state(cs) < 0)
         return -1;
   }

   if (cs->dirty)
      return -2;

   MD5Init(&md5Ctx);
   MD5Update(&md5Ctx, (unsigned char*) &cs->state, sizeof(cs->state));
   MD5Final(md5, &md5Ctx);

   // If checksum fails, reload state
   if (memcmp(md5, &cs->md5, sizeof(md5))) {
      if (load_critical_state(cs) < 0)
         return -3;

      MD5Init(&md5Ctx);
      MD5Update(&md5Ctx, (unsigned char*) &cs->state, sizeof(cs->state));
      MD5Final(md5, &md5Ctx);
      if (memcmp(md5, &cs->md5, sizeof(md5)))
         return -4;
   }

   // Copy the critical state back to the caller
   if (len > CRITICAL_STATE_MAX_LEN)
      len = CRITICAL_STATE_MAX_LEN;

   memcpy(state, &cs->state, len);

   return len;
}
