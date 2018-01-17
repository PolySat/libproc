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
#include <proclib.h>
#include <ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <util.h>
#include <getopt.h>

int usage(const char *app)
{
   printf("Usage: %s -d <dir> [-c <count>]\n"
          "  -d <dir> path to directory to clean\n"
          "  -c <count> maximum number of files to keep in directory\n"
          "  -h keep all hidden files, regardless of count\n"
   , app);

   return 0;
}

static void del_cb(const char *file, void *arg)
{
   printf("Deleted %s\n", file);
}

int main(int argc, char **argv)
{
   int opt;
   char *dir = NULL;
   int count = 10;
   int ignore_hidden = 0;

#ifdef __APPLE__
   optind = 1;
#else
   optind = 0;
#endif

   while ((opt = getopt(argc, argv, "d:c:h")) != -1) {
      switch(opt) {
         case 'd':
            dir = optarg;
            break;
         case 'c':
            count = atol(optarg);
            break;
         case 'h':
            ignore_hidden = 1;
            break;
         default:
            return usage(argv[0]);
      }
   }

   if (optind < (argc) || !dir)
      return usage(argv[0]);

   UTIL_cleanup_dir(dir, count, ignore_hidden, NULL, NULL, del_cb, NULL);

   return 0;
}
