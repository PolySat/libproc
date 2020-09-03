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
 * @file debug.c Debug interface source file.
 *
 * Source for the code to manage the debug/error interface in the polysat
 * library.
 *
 * @author John Bellardo
 * @author Greg Manyak
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "eventTimer.h"

/// Message buffer length
#define MESSAGE_BUFF_LENGTH 1024

/// Default level of debug message printing
static int gDBGLevel = DBG_LEVEL_WARN;
static struct EventTimer *gDBGTimer = NULL;

// Debug output function
void DBG_print(int level, const char *fmt, ...)
{
   va_list ap;
   char buff[MESSAGE_BUFF_LENGTH];
   struct timeval now;

   if (level > gDBGLevel) {
      return;
   }

   // Read in the variable arguments and print the message to the log
   va_start(ap, fmt);
   if (gDBGTimer) {
      vsnprintf(&buff[0], sizeof(buff), fmt, ap);
      buff[sizeof(buff)-1] = 0;

      gDBGTimer->get_gmt_time(gDBGTimer, &now);
      syslog(level, "%lu.%06lu: %s", now.tv_sec, now.tv_usec, buff);
   }
   else
      vsyslog(level, fmt, ap);

   va_end(ap);
}

// Modify level of allowed debug output
void DBG_setLevel(int newLevel)
{
   gDBGLevel = newLevel;
   setlogmask(LOG_UPTO(newLevel));
}

// Initialize debug interface
void DBG_init(const char * procName)
{
   openlog(procName, LOG_CONS | LOG_PID | LOG_PERROR, LOG_USER);
}

void DBG_set_timer(struct EventTimer *timer)
{
   gDBGTimer = timer;
}

// Print out a message with the system error number message
void DBG_syserr(unsigned long line, const char *func, const char *file,
      int level, const char *fmt, ...)
{
   char buff[MESSAGE_BUFF_LENGTH];
   struct timeval now;
   va_list ap;

   if (level > gDBGLevel) {
      return;
   }

   // Read in the variable arguments to the buffer
   va_start(ap, fmt);
   vsnprintf(&buff[0], sizeof(buff), fmt, ap);
   buff[sizeof(buff)-1] = 0;
   va_end(ap);

   if (gDBGTimer) {
      gDBGTimer->get_gmt_time(gDBGTimer, &now);
      syslog(level, "%lu,%06lu: %m - %s in %s() at %s:%lu\n",
            now.tv_sec, now.tv_usec, buff, func, file, line);
   }
   else
      syslog(level, "%m - %s in %s() at %s:%lu\n",
         buff, func, file, line);
}
