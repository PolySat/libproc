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

#include "telm_dict.h"
#include "ipc.h"
#include "priorityQueue.h"
#include "debug.h"

int TELM_print_datalogger_info(struct TELMTelemetryInfo *points,
   const char *dfl_name, const char *dfl_path, int argc, char **argv)
{
   int opt;
   int header = 0;
   int mode = 0;
   int start_time = 5;
   int period = 300;
   struct TELMTelemetryInfo *curr;
   const char *name = dfl_name;
   const char *path = dfl_path;
   struct StringList {
      const char *name;
      struct StringList *next;
   } *itr, *head = NULL;

#ifdef __APPLE__
   optind = 1;
#else
   optind = 0;
#endif

   while ((opt = getopt(argc, argv, "s:n:x:m:t:p:H")) != -1) {
      switch(opt) {
         case 'p':
            period = atol(optarg);
            break;
         case 's':
            start_time = atol(optarg);
            break;
         case 't':
            path = optarg;
            break;
         case 'H':
            header = 1;
            break;
         case 'n':
            name = optarg;
            break;
         case 'x':
            itr = malloc(sizeof(*itr));
            itr->next = head;
            itr->name = optarg;
            head = itr;
            break;
         case 'm':
            mode = atol(optarg);
            break;
         default:
            goto usage;
      }
   }

   if (optind < (argc))
      goto usage;

   if (mode == 2) {
      printf("%s\n", name);
      return 0;
   }

   if (header)
      printf("<EVENT>\n"
             "   NAME=%s\n"
             "   PROC_NAME=%s\n"
             "   NUM=0\n"
             "   START_TIME=P+%d\n"
             "   SCHED_TIME=%d\n"
             "   DURATION=0\n"
             "</EVENT>\n\n"
             "<SUBPROCESS>\n"
             "   NAME=%s\n"
             "   PROC_PATH=%s\n"
             "   POWER=1\n"
             "   POWER_PROC=./none\n"
             "   PARAM=\n\n",
             name, name, start_time, period, name, path);

   for (curr = points; curr->id; curr++) {
      int ignore = 0;
      for (itr = head; itr; itr = itr->next) {
         if (0 == strcasecmp(itr->name, curr->id)) {
            ignore = 1;
            break;
         }
      }

      if (mode == 1) {
         if (!ignore)
            printf("%s\n", curr->id);
      }
      else {
         if (!ignore) {
            printf("   <SENSOR>\n");
            printf("      NAME=%s\n", name);
            printf("      SENSOR_KEY=%s\n", curr->id);
            printf("      LOCATION=%s\n", curr->location);
            printf("      GROUPS=%s\n", curr->group);
            printf("   </SENSOR>\n");
         }
         else {
            printf("   <IGNORE>\n");
            printf("      SENSOR_KEY=%s\n", curr->id);
            printf("   </IGNORE>\n");
         }
      }
   }

   if (header)
      printf("</SUBPROCESS>\n");

   return 0;

usage:
   printf("%s [-n <subprocess name>] [-H] [-p <period>] [-s <start time>] [-t <telemetry path] [-x <exclude tlm id>] [-x <>] ...\n",
      argv[0]);
   return 0;

}

int TELM_print_sensor_metadata(struct TELMTelemetryInfo *points,
   struct TELMEventInfo *events)
{
   struct TELMTelemetryInfo *curr;
   struct TELMBitfieldInfo *bitr;
   struct TELMEventInfo *eitr;
   const char *type;

   for (curr = points; curr->id; curr++) {
      type = "SENSOR";
      if (curr->computed_by)
         type = "VSENSOR";

      printf("<%s>\n", type);
      printf("   KEY=%s\n", curr->id);
      printf("   LOCATION=%s\n", curr->location);
      printf("   SUBSYSTEM=%s\n", curr->group);
      printf("   UNITS=%s\n", curr->units);
      printf("   DIVISOR=%lf\n", curr->divisor);
      printf("   OFFSET=%lf\n", curr->offset);
      printf("   NAME=%s\n", curr->name);
      printf("   DESCRIPTION=%s\n", curr->desc);
      if (curr->computed_by)
      printf("   FUNCTION=%s\n", curr->computed_by);

      if (curr->bitfields) {
         for (bitr = curr->bitfields; bitr->set_label; bitr++) {
            printf("   <ENUM>\n");
            printf("      VALUE=%u\n", bitr->value);
            printf("      LABEL=%s\n", bitr->set_label);
            if (bitr->clear_label)
               printf("      UNSET=%s\n", bitr->clear_label);
            printf("   </ENUM>\n");
         }
      }

      printf("</%s>\n", type);
   }

   for (eitr = events; eitr->port_name; ++eitr) {
      printf("<EVENT>\n");
      printf("   PORT_NAME=%s\n", eitr->port_name);
      printf("   PORT=%u\n", socket_get_addr_by_name(eitr->port_name));
      printf("   ID=%u\n", eitr->id);
      printf("   NAME=%s\n", eitr->name);
      printf("   DESCRIPTION=%s\n", eitr->desc);
      printf("</EVENT>\n");
   }

   return 0;
}

int TELM_print_json_telem_dict(struct TELMTelemetryInfo *points,
   struct TELMEventInfo *events, int argc, char **argv)
{
   int opt;
   int first = 1;
   int mode = 0;
   struct TELMTelemetryInfo *curr;
   struct StringList {
      const char *name;
      struct StringList *next;
   } *itr, *head = NULL;
   struct TELMEventInfo *eitr;
   struct TELMBitfieldInfo *bitr;
   const char *type;

#ifdef __APPLE__
   optind = 1;
#else
   optind = 0;
#endif

   while ((opt = getopt(argc, argv, "x:m:")) != -1) {
      switch(opt) {
         case 'x':
            itr = malloc(sizeof(*itr));
            itr->next = head;
            itr->name = optarg;
            head = itr;
            break;
         case 'm':
            mode = atol(optarg);
            break;
         default:
            goto usage;
      }
   }

   if (optind < (argc))
      goto usage;

   first = 1;
   for (curr = points; curr->id; curr++) {
      int ignore = 0;

      for (itr = head; itr; itr = itr->next) {
         if (0 == strcasecmp(itr->name, curr->id)) {
            ignore = 1;
            break;
         }
      }
      if (ignore)
         continue;

      if (mode == 1 && !curr->computed_by)
         type = "sensor";
      else if (mode == 3 && curr->computed_by)
         type = "virtual_sensor";
      else
         continue;

      if (!first)
         printf(",\n");
      printf("{\n");
      printf("   \"type\": \"%s\",\n", type);
      printf("   \"key\": \"%s\",\n", curr->id);
      printf("   \"location\": \"%s\",\n", curr->location);
      printf("   \"subsystem\": \"%s\",\n", curr->group);
      printf("   \"units\": \"%s\",\n", curr->units);
      printf("   \"divisor\": %lf,\n", curr->divisor);
      printf("   \"offset\": %lf,\n", curr->offset);
      printf("   \"name\": \"%s\",\n", curr->name);
      if (curr->computed_by)
      printf("   \"function\": \"%s\",\n", curr->computed_by);

      if (curr->bitfields) {
         int efirst = 1;
         printf("   \"enums\": [\n");
         for (bitr = curr->bitfields; bitr->set_label; bitr++) {
            if (!efirst)
               printf(",\n");

            printf("      {\n");
            printf("         \"value\": %u,\n", bitr->value);
            if (bitr->clear_label)
               printf("         \"unset\": \"%s\",\n", bitr->clear_label);
            printf("         \"label\": \"%s\"\n", bitr->set_label);
            printf("   }");

            efirst = 0;
         }
         printf("   ],\n");
      }

      printf("   \"description\": \"%s\"\n", curr->desc);

      printf("}");
      first = 0;
   }

   for (eitr = events; mode == 2 && eitr->id; eitr++) {
      if (!first)
         printf(",\n");
      printf("{\n");

      printf("   \"port_name\" : \"%s\",\n", eitr->port_name);
      printf("   \"port\" : %u,\n", socket_get_addr_by_name(eitr->port_name));
      printf("   \"id\" : =%u,\n", eitr->id);
      printf("   \"name\" : \"%s\",\n", eitr->name);
      printf("   \"description\" : \"%s\"\n", eitr->desc);

      printf("}");
      first = 0;
   }
   if (!first)
      printf("\n");

   return 0;

usage:
   printf("%s [-m <mode>] [-x <exclude tlm id>] [-x <>] ...\n",
      argv[0]);
   return 0;
}
