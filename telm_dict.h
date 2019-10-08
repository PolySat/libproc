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
#ifndef TELM_DICT_H
#define TELM_DICT_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Support routines for generating telemetry dictionaries, datalogger
 * configuration files, and other related telemetry info.  The basic idea is
 * the developer fills out a set of static data structures containing the
 * necessary metadata, and then calls the correct generation function
 * passing in a pointer to the static structure
 **/

struct TELMBitfieldInfo {
   uint32_t value;
   const char *set_label;
   const char *clear_label;
};

struct TELMTelemetryInfo {
   const char *id;
   const char *location;
   const char *group;
   const char *units;
   double divisor;
   double offset;
   const char *name;
   const char *desc;
   struct TELMBitfieldInfo *bitfields;
   const char *computed_by;
};

struct TELMEventInfo {
   const char *port_name;
   uint16_t id;
   const char *name;
   const char *desc;
};

/*** Print the telemetry information in a format for datalogger ***/
extern int TELM_print_datalogger_info(struct TELMTelemetryInfo *points,
   const char *dl_name, const char *telem_path, int argc, char **argv);

/** Print the telemetry information in a format for use in the
 *  telemetry export process
 **/
extern int TELM_print_sensor_metadata(struct TELMTelemetryInfo *points,
   struct TELMEventInfo *events);

/*** Print the telemetry in JSON format, intended for use in 
 *   auto-generated documentation and other programs.
 ***/
extern int TELM_print_json_telem_dict(struct TELMTelemetryInfo *points,
   struct TELMEventInfo *events, int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif
