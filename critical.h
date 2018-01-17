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
#ifndef CRITICAL_H
#define CRITICAL_H

#include "md5.h"
#include <limits.h>

struct CSFileState {
   char prefix[9];
   char *curr_file;
   char *directory;
   int generation;
};

#define CRITICAL_STATE_MAX_LEN 224
#define CRITICAL_STATE_NUM_FILES 2

struct CSState {
   uint64_t state_version;
   const char *name;
   int dirty;
   struct CSFileState files[CRITICAL_STATE_NUM_FILES];
   uint8_t state[CRITICAL_STATE_MAX_LEN];
   unsigned char md5[MD5_DIGEST_LENGTH];
};

extern void critical_state_init(struct CSState *cs, const char *name);
extern void critical_state_cleanup(struct CSState *cs);

#endif
