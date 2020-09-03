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
 * @file debug.c Debug interface header file.
 *
 * @author John Bellardo <bellardo@calpoly.edu>
 * @author Greg Manyak <gregmanyak@gmail.com>
 */
#ifndef DEBUG_H
#define DEBUG_H

#include <errno.h>
#include <syslog.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Debug level for absolutely no output
#define DBG_LEVEL_NONE  LOG_EMERG
/// Debug level that reports fatal errors only
#define DBG_LEVEL_FATAL LOG_ERR
/// Debug level that reports non-fatal warnings and all errors
#define DBG_LEVEL_WARN  LOG_WARNING
/// Debug level that includes errors and informative output
#define DBG_LEVEL_INFO  LOG_INFO
/// Debug level that includes all possible output
#define DBG_LEVEL_ALL   LOG_DEBUG

/**
 *  Issues a debug/error message with a given priority to the log.
 *
 *  Only messages that are at or above the configured priority will be
 *  printed to the screen and the log.
 *
 *  @param  level Priority of message (DBG_LEVEL_FATAL up to DBG_LEVEL_INFO).
 *  @param  fmt   Debug message string.
 *  @param  ...   Parameters of message (like arguments to printf).
 */
void DBG_print(int level, const char *fmt, ...);

/**
 * Sets the level of the debug messages. Any messages up to and including this
 * priority will be printed to the screen and sent to the log.
 *
 * @param   newLevel Allowed level of debug messages.
 */
void DBG_setLevel(int newLevel);

/**
 * Initializes the debug interface with the name of the process.
 * Process name and PID will be printed with each message and they will be sent
 * to both the log (located at /var/log/messages) and the console.
 *
 * @param   procName Name of process, to be included with each message.
 */
void DBG_init(const char *procName);

struct EventTimer;
/**
 * Provides a timer to use when timestamping debug messages.  Optional.
 *
 * @param   timer The timer to use for message timestamps.  Pass in NULL
 *                to disable timestamps.
 */
void DBG_set_timer(struct EventTimer *timer);

/**
 * Message logging utilizing the built in error messages associated with errno.
 *
 * @param   line  Line of the error in the code.
 * @param   func  Name of the function.
 * @param   file  Name of the source file.
 * @param   level Message priority level.
 * @param   fmt   Error message.
 * @param   ...   Parameters of message (like arguments of printf).
 */
void DBG_syserr(unsigned long line, const char *func, const char *file,
      int level, const char *fmt, ...);

/**
 * \brief A macro for standard debugging during development, should be used
 *  in place of printf and fprintf(stderr, ...).
 */
#define DBG(...) do { DBG_print(DBG_LEVEL_INFO, __VA_ARGS__); } while(0)

/**
 * \brief Convenient debug interface initialization for development. Enables
 *  all levels of debug messaging.
 */
#define DBG_INIT() do { DBG_setLevel(DBG_LEVEL_ALL); } while (0)

/**
 * \brief A macro that reports a system error
 *  call failed it uses the debugging facility to print a formated error
 *  message at the given logging level.  It simply calls the DBG_syserr
 *  function with the expanded parameter list.
 *  The parameters are treated just like arguments to printf.
 */
#define ERR_REPORT(lvl, ...) do { DBG_syserr(__LINE__, __FUNCTION__, \
                        __FILE__, (lvl), __VA_ARGS__); } while(0)

/**
 * \brief A macro that verifies a system call worked successfully.  If the
 *  call failed it uses the debugging facility to print an error and exit
 *  the program.  The parameters are treated just like arguments to printf.
 *
 *  Use of ERR_EXIT (see below) is recommended instead.
 */
#define ERRNO_EXIT(...) do { if (errno) { ERR_REPORT(DBG_LEVEL_FATAL, \
                                 __VA_ARGS__); exit(1); } } while(0)

/**
 * \brief This macro is the same as ERRNO_EXIT but requires the actual command as a
 *  parameter so that errno can be reset before the function call.
 */
#define ERR_EXIT(cmd, ...) do { errno = 0; (cmd); ERRNO_EXIT( \
                                 __VA_ARGS__); } while(0)

/**
 * \brief A macro that verifies a system call worked successfully.  If the
 *  call failed it uses the debugging facility to print a warning.  The
 *  parameters are treated just like arguments to printf.
 *  Errno is reset and execution continues after the warning.
 *
 *  Use of ERR_WARN (see below) is recommended instead.
 */
#define ERRNO_WARN(...) do { if (errno) { ERR_REPORT(DBG_LEVEL_WARN, \
                           __VA_ARGS__); errno = 0; } } while(0)

/**
 * \brief This macro is the same as ERRNO_WARN but requires the actual command as a
 *  parameter so that errno can be reset before the function call.
 */
#define ERR_WARN(cmd, ...) do { errno = 0; (cmd); ERRNO_WARN( \
                                          __VA_ARGS__); }  while(0)

#ifdef __cplusplus
}
#endif

#endif
