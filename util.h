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
#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Returns the serial numner for the board, or 0 if running on the
//  dev board
extern unsigned long UTIL_board_sn(void);

#define BOARD_TYPE_UNKNOWN 0x1234
#define BOARD_TYPE_INTREPID 0x7315
#define BOARD_TYPE_DEV 0xDEAD

/// Returns an enumerated type indicating which type of board this is
extern int UTIL_board_type(void);

typedef struct timeval (*cleanup_mod_time_cb_t)(const struct dirent *dirent,
   const char *full_path, struct stat *statbuff, void *arg);

typedef void (*post_delete_cb_t)(const char *file_path, void *arg);

/** Deletes old files from a directory as part of normal housekeeping

**/
int UTIL_cleanup_dir(const char *dirname, int max_entries, int ignore_hidden,
    cleanup_mod_time_cb_t modtime_cb, void *cb_arg, post_delete_cb_t del_cb,
    void *del_cb_arg);

/** Verifies that dir exists and is a directory **/
extern int UTIL_ensure_dir(const char *dir);

/** Verifies that dir exists and is a directory.  If it doesn't exist
    it will attempt to create it.  Returns true if successful,
    false otherwise
 **/
extern int UTIL_ensure_path(const char *dir);

/** Support routines for generating telemetry dictionaries, datalogger
 * configuration files, and other related telemetry info.  The basic idea is
 * the developer fills out a set of static data structures containing the
 * necessary metadata, and then calls the correct generation function
 * passing in a pointer to the static structure
 **/

struct UTILBitfieldInfo {
   uint32_t value;
   const char *set_label;
   const char *clear_label;
};

struct UTILTelemetryInfo {
   const char *id;
   const char *location;
   const char *group;
   const char *units;
   uint32_t divisor;
   int32_t offset;
   const char *name;
   const char *desc;
   struct UTILBitfieldInfo *bitfields;
   const char *computed_by;
};

struct UTILEventInfo {
   const char *port_name;
   uint16_t id;
   const char *name;
   const char *desc;
};

/*** Print the telemetry information in a format for datalogger ***/
extern int UTIL_print_datalogger_info(struct UTILTelemetryInfo *points,
   const char *dl_name, int argc, char **argv);

/** Print the telemetry information in a format for use in
 *  telemetry export process
 **/
extern int UTIL_print_sensor_metadata(struct UTILTelemetryInfo *points,
   struct UTILEventInfo *events);

/*** Print the telemetry in html format ***/
extern int UTIL_print_html_telem_dict(struct UTILTelemetryInfo *points,
   struct UTILEventInfo *events);

#ifdef __cplusplus
}
#endif

#endif
