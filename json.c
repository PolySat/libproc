#include "json.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define is_whitespace(c) ((c)==' ' || (c)=='\t' || (c)=='\n' || (c)=='\r')

typedef void (*json_itr_cb)(const char *prop, const char *val, void*);

enum JSONParserMode { START_OBJ, END_OBJ, START_PROP, END_PROP, END_JSON,
    KEY_QUOTED_ACCUM, VAL_QUOTED_ACCUM, KVP_SEP, START_VAL, VAL_ACCUM };
enum JSONParserResult { PARSE_GOOD, PARSE_NO_ROOT_OBJ, PARSE_STRAY_OBJ,
   PARSE_UNKNOWN_ERR, PARSE_STRAY_END_OBJ, PARSE_STRAY_QUOTE,
   PARSE_STRAY_COLIN};

static void json_setup_cb(const char *key_start, const char *key_end,
      const char *val_start, const char *val_end, json_itr_cb itr, void *arg)
{
   char key[512], val[2048];
   int len;

   len = key_end - key_start - 1;
   if (len >= sizeof(key))
      len = sizeof(key) - 1;

   memcpy(key, key_start + 1, len);
   key[len] = 0;

   if (*val_start == '"') {
      len = val_end - val_start - 1;
      if (len >= sizeof(val))
         len = sizeof(val) - 1;

      memcpy(val, val_start + 1, len);
   }
   else {
      len = val_end - val_start + 1;
      if (len >= sizeof(val))
         len = sizeof(val) - 1;

      memcpy(val, val_start, len);
   }
   val[len] = 0;

   itr(key, val, arg);
}

static enum JSONParserResult json_iterate_props(const char *json,
      json_itr_cb itr, void *arg)
{
   enum JSONParserMode mode = START_OBJ;
   const char *curr, *key_start = NULL, *key_end = NULL;
   const char *val_start = NULL, *val_end = NULL;

   for (curr = json; curr && *curr; curr++) {
      // printf("%02X: %d\n", *curr, (int)mode);
      switch (*curr) {
         case ' ':
         case '\t':
         case '\r':
         case '\n':
            if (mode == VAL_ACCUM) {
               val_end = curr - 1;
               mode = END_PROP;
               json_setup_cb(key_start, key_end, val_start, val_end, itr, arg);
            }
            break;

         case '{':
            if (mode == START_OBJ)
               mode = START_PROP;
            else if (mode != KEY_QUOTED_ACCUM && mode != VAL_QUOTED_ACCUM)
               return PARSE_STRAY_OBJ;
            break;

         case '}':
            if (mode == VAL_ACCUM) {
               val_end = curr - 1;
               mode = END_PROP;
               json_setup_cb(key_start, key_end, val_start, val_end, itr, arg);
            }

            if (mode == START_PROP || mode == END_PROP)
               mode = END_JSON;
            else if (mode != KEY_QUOTED_ACCUM && mode != VAL_QUOTED_ACCUM)
               return PARSE_STRAY_END_OBJ;
            break;

         case '"':
            if (mode == START_PROP) {
               mode = KEY_QUOTED_ACCUM;
               key_start = curr;
               key_end = val_start = val_end = NULL;
            }
            else if (mode == KEY_QUOTED_ACCUM) {
               mode = KVP_SEP;
               key_end = curr;
            }
            else if (mode == START_VAL) {
               val_start = curr;
               mode = VAL_QUOTED_ACCUM;
            }
            else if (mode == VAL_QUOTED_ACCUM) {
               val_end = curr;
               mode = END_PROP;
               json_setup_cb(key_start, key_end, val_start, val_end, itr, arg);
            }
            else
               return PARSE_STRAY_QUOTE;
            break;

         case ',':
            if (mode == VAL_ACCUM) {
               val_end = curr - 1;
               mode = END_PROP;
               json_setup_cb(key_start, key_end, val_start, val_end, itr, arg);
            }

            if (mode == END_PROP) {
               mode = START_PROP;
            }
            break;

         case ':':
            if (mode == KVP_SEP)
               mode = START_VAL;
            else if (mode != KEY_QUOTED_ACCUM && mode != VAL_QUOTED_ACCUM)
               return PARSE_STRAY_COLIN;
            break;

         default:
            if (mode == START_VAL) {
               val_start = curr;
               mode = VAL_ACCUM;
            }
            break;
      }
   }

   switch (mode) {
      case END_JSON:
         return PARSE_GOOD;
      case START_OBJ:
         return PARSE_NO_ROOT_OBJ;
      default:
         break;
   }

   return PARSE_UNKNOWN_ERR;
}

struct JStrItr {
   const char *prop;
   const char *val;
};

static void json_str_itr_cb(const char *prop, const char *val, void *arg)
{
   struct JStrItr *params = (struct JStrItr*)arg;

   if (params && params->prop && !strcmp(params->prop, prop))
      params->val = val;
}

const char *json_get_string_prop(const char *json, const char *prop)
{
   struct JStrItr params;
   enum JSONParserResult err;

   if (!json || !prop)
      return NULL;

   params.prop = prop;
   params.val = NULL;

   err = json_iterate_props(json, json_str_itr_cb, &params);
   if (err != PARSE_GOOD) {
      printf("JSON Parsing failed with error %d\n", err);
      return NULL;
   }

   return params.val;
}
