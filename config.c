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
 * Source file for config file management.
 * @author John Bellardo
 */

#include <limits.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "config.h"
#include "debug.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

struct CFG_buffered_obj
{
   void *parsed_cfg;
   char *buff;
   size_t len;
   struct CFG_ParseValue val;
};

static char gConfigPath[PATH_MAX+1] = "";

const char *CFG_getPath()
{
   return gConfigPath;
}

int CFG_locateConfigFile(const char *name)
{
   char path[PATH_MAX+1];
   struct stat sb;

   strcpy(path, name);
   if (!stat(path, &sb)) {
      strcpy(gConfigPath, path);
      DBG_print(DBG_LEVEL_INFO, "Using config file: %s\n", gConfigPath);
      return 1;
   }

   if (getenv("HOME")) {
      sprintf(path, "%s/.%s", getenv("HOME"), name);
      if (!stat(path, &sb)) {
         strcpy(gConfigPath, path);
         DBG_print(DBG_LEVEL_INFO, "Using config file: %s\n", gConfigPath);
         return 1;
      }
   }

   sprintf(path, "/etc/%s", name);
   if (!stat(path, &sb)) {
      strcpy(gConfigPath, path);
      DBG_print(DBG_LEVEL_INFO, "Using config file: %s\n", gConfigPath);
      return 1;
   }

   sprintf(path, "/usr/local/etc/%s", name);
   if (!stat(path, &sb)) {
      strcpy(gConfigPath, path);
      DBG_print(DBG_LEVEL_INFO, "Using config file: %s\n", gConfigPath);
      return 1;
   }

   DBG_print(DBG_LEVEL_WARN, "Expected name %s\n", name);
   DBG_print(DBG_LEVEL_WARN, "Failed to locate config file %s\n", gConfigPath);
   return 0;
}

struct CFG_Context {
   FILE *file;
   int lineNum;
   char *accum;
   size_t used, len;
   char *replay;
   int stop;
   int allow_buffering;
   char buff[1024*4];
};

/**
  * \brief Read the next real line from the configuration file.  It skips empty
  * lines and lines with the comment character in the first column.  It also
  * removes all trailing newline and carrage return characters, and all leading
  * whitespace before returning the line.
  * \returns true if a line was read successfully, and false if the file is
  * done.
  **/
static int NextLine(struct CFG_Context *ctx)
{
   int len, start, i;
   int skipFlag = 0; //If full line is not read in, skip last bit of junk
   char *sep;

   if (ctx->replay) {
      sep = strchr(ctx->replay, '\n');
      if (sep)
         *sep++ = 0;
      strncpy(ctx->buff, ctx->replay, sizeof(ctx->buff));
      ctx->buff[sizeof(ctx->buff) - 1] = 0;
      ctx->replay = sep;
      if (!sep)
         ctx->stop = 1;
      ctx->lineNum++;
      return 1;
   }
   if (ctx->stop)
      return 1;

   for ( ; NULL != fgets(ctx->buff, sizeof(ctx->buff), ctx->file);
               ++ctx->lineNum) {
      len = strlen(ctx->buff);
      if (0 == len)
         continue;

      if (ctx->buff[len-1] != '\n' || skipFlag) {
         DBG_print(DBG_LEVEL_WARN, "Line %d in config file %s too long, "
               "ignoring\n", ctx->lineNum, gConfigPath);
         if(skipFlag)
            skipFlag = 0; //Only skip once
         else
            skipFlag = 1; //Skip the rest of the incomplete line next time
         continue;
      }

      while(len && (ctx->buff[len-1] == '\n' || ctx->buff[len-1] == '\r') )
         ctx->buff[--len] = 0;
      for(start = 0; start < len && isblank(ctx->buff[start]); start++)
         ;
      for (i = start; start && (i <= len); i++)
         ctx->buff[i-start] = ctx->buff[i];

      if (ctx->buff[0] == 0 || ctx->buff[0] == '#' || ctx->buff[0] == ';')
         continue;
      ++ctx->lineNum;
      return 1;
   }
   return 0;
}

//Returns NULL on error
static void *processKeyValueLine(struct CFG_Context *ctx,
      struct CFG_ParseValue *keys, void *data)
{
   char *key, *value;
   struct CFG_ParseValue *curr;
   int found = 0;

   key = ctx->buff;
   value = strchr(key, '=');
   if (value)
      *value++ = 0;

   for(curr = keys; curr && curr->name; curr++) {
      if (!strcasecmp(curr->name, key)) {
         found = 1;
         if (curr->lookup_cb) {
            DBG_print(DBG_LEVEL_WARN, "Warning: Option name '%s' can not be a "
                  "sub-object at line %d\n", key, ctx->lineNum);
         } else {
            if (curr->cb.cb)
               data = (*curr->cb.cb)(key, value, data,curr->cb.p1, curr->cb.p2);
         }
         break;
      }
   }
   if (!found)
      DBG_print(DBG_LEVEL_WARN, "Warning: Option '%s' not valid at line %d\n",
            key, ctx->lineNum);

   return data;
}

static void accumulate_line(struct CFG_Context *ctx)
{
   int len = strlen(ctx->buff);

   if (!ctx->accum || !ctx->len) {
      ctx->len = 1024;
      ctx->accum = malloc(ctx->len);
      ctx->used = 0;
   }

   if (len >= ctx->len - ctx->used - 3) {
      ctx->len *= 2;
      ctx->accum = realloc(ctx->accum, ctx->len);
   }

   memcpy(&ctx->accum[ctx->used], ctx->buff, len);
   ctx->used += len;
   ctx->accum[ctx->used++] = '\n';
   ctx->accum[ctx->used] = 0;
}

static void *ParseObject(struct CFG_Context *ctx, struct CFG_ParseObj *parObj,
      void *parData)
{
   char *obj = &ctx->buff[1];
   char *params;
   char *objLcl, *paramsLcl = NULL;
   int more;
   struct CFG_ParseValue *curr = NULL;
   struct CFG_ParseObj *child = NULL;
   void *data = NULL;
   char subobj_type_buff[256];
   int buffer_obj = ctx->allow_buffering;
   struct CFG_buffered_obj *obj_buff = NULL;

   subobj_type_buff[0] = 0;
   if (obj[strlen(obj)-1] != '>') {
      DBG_print(DBG_LEVEL_WARN, "Config line '%s' lacking trailing '>' line "
            "%d\n", ctx->buff, ctx->lineNum);
      return NULL;
   }

   obj[strlen(obj) - 1] = 0;
   params = strchr(obj, ' ');
   if (params)
      *params++ = 0;

   if( !(objLcl = strdup(obj)) ){
      ERRNO_WARN("strdup error:");
      return NULL;
   }
   if (params){
      if( !(paramsLcl = strdup(params)) ){
         ERRNO_WARN("strdup error:");
         return NULL;
       }
   }
   else{
      if( !(paramsLcl = strdup("")) ){
         ERRNO_WARN("strdup error:");
         return NULL;
       }
   }

   if (parObj && !ctx->accum) {
      for(curr = parObj->keys; curr->name; curr++) {
         if (!strcasecmp(obj, curr->name))
            break;
      }
      if (!curr->name)
         curr = NULL;
      if (curr && !curr->lookup_cb) {
         DBG_print(DBG_LEVEL_WARN, "Warning: Option %s not a sub"
               "-object ,line %d\n", obj, ctx->lineNum);
         curr = NULL;
      }
      else if (curr && curr->lookup_cb) {
         snprintf(subobj_type_buff, sizeof(subobj_type_buff), "__type_%s", obj);
         subobj_type_buff[sizeof(subobj_type_buff)-1] = 0;
         child = curr->lookup_cb(paramsLcl, curr->lookup_arg, &buffer_obj);
         if (buffer_obj) {
            obj_buff = (struct CFG_buffered_obj*) malloc(sizeof(*obj_buff));
            memset(obj_buff, 0, sizeof(*obj_buff));
            obj_buff->val = *curr;
         }
      }
   }

   if (ctx->accum || (!child && obj_buff)) {
      char *oterm = &obj[strlen(obj)];
      if (params) {
         *oterm = ' ';
         params[strlen(params)] = '>';
      }
      else
         *oterm = '>';

      accumulate_line(ctx);
      *oterm = 0;
      if (params)
         params[strlen(params) - 1] = 0;
   }

   if (!child && !ctx->accum)
      DBG_print(DBG_LEVEL_WARN, "Warning: Description for '%s/%s' not found, "
            "line %d\n", obj, paramsLcl, ctx->lineNum);

   if (child && child->initCb.cb)
      data = (*child->initCb.cb)(objLcl, paramsLcl, data,
            child->initCb.p1, child->initCb.p2);

   if (paramsLcl) {
      struct CFG_ParseValue *currLine = NULL;
      if (parObj) {
         for(currLine = parObj->keys; currLine && currLine->name; currLine++) {
            if (!strcasecmp(currLine->name, subobj_type_buff)) {
               if (currLine->cb.cb)
                  (*currLine->cb.cb)(subobj_type_buff, paramsLcl, parData,
                              currLine->cb.p1, currLine->cb.p2);
               break;
            }
         }
      }

      if (child) {
         for(currLine = child->keys; currLine && currLine->name; currLine++) {
            if (!strcasecmp(currLine->name, "__type")) {
               if (currLine->cb.cb)
                  (*currLine->cb.cb)("__type", paramsLcl, data,
                              currLine->cb.p1, currLine->cb.p2);
               break;
            }
         }
      }
   }

   while((more = NextLine(ctx)) ) {
      obj = ctx->buff;

      if (obj[0] == '<' && obj[1] == '/')  {
         if (ctx->accum)
            accumulate_line(ctx);
         obj[strlen(obj) - 1] = 0;
         if (strlen(obj) < 3 || strcasecmp(&obj[2], objLcl)) {
            DBG_print(DBG_LEVEL_WARN, "Warning: Malformed end object line in "
                  "config file: %s\n", obj);
            continue;
         }
         break;
      }

      if (obj[0] == '<') {
         buffer_obj = ctx->allow_buffering;
         ctx->allow_buffering = 1;
         data = ParseObject(ctx, child, data);
         ctx->allow_buffering = buffer_obj;
         continue;
      }

      if (!ctx->accum)
         data = processKeyValueLine(ctx, child ? child->keys : NULL, data);
      else
         accumulate_line(ctx);
   }
   if (!more) {
      DBG_print(DBG_LEVEL_WARN, "Warning: permature end of config file\n");
   }

   if (child && child->finiCb.cb)
      data = (*child->finiCb.cb)(objLcl, paramsLcl, data,
            child->finiCb.p1, child->finiCb.p2);

   if (obj_buff) {
      if (child)
         obj_buff->parsed_cfg = data;
      else {
         obj_buff->len = ctx->used;
         obj_buff->buff = ctx->accum;
         ctx->accum = NULL;
         ctx->len = ctx->used = 0;
      }
      data = obj_buff;
   }

   free(objLcl);
   if (paramsLcl)
      free(paramsLcl);

   if (curr && curr->cb.cb)
      data = (*curr->cb.cb)(curr->name, data, parData,
            curr->cb.p1, curr->cb.p2);
   else
      data = parData;

   return data;
}

void *CFG_cfg_for_object_buffer(struct CFG_buffered_obj *obj)
{
   struct CFG_ParseObj fake_root;
   struct CFG_ParseValue fake_keys[2];
   struct CFG_Context ctx;

   if (!obj)
      return NULL;

   if (obj->parsed_cfg)
      return obj->parsed_cfg;

   memset(&ctx, 0, sizeof(ctx));
   memset(&fake_root, 0, sizeof(fake_root));
   memset(&fake_keys, 0, sizeof(fake_keys));
   fake_keys[0] = obj->val;
   fake_root.keys = fake_keys;

   fake_keys[0].cb.cb = &CFG_PtrCpyCB;
   fake_keys[0].cb.p1 = NULL;
   fake_keys[0].cb.p2 = NULL;

   ctx.replay = obj->buff;

   if (NextLine(&ctx)) {
      ParseObject(&ctx, &fake_root, &obj->parsed_cfg);
   }
   if (obj->buff)
      free(obj->buff);
   obj->buff = NULL;

   return obj->parsed_cfg;
}

void *CFG_parseFile(struct CFG_ParseObj *rootObj)
{
   return CFG_parseFileAtPath(rootObj, gConfigPath);
}

void *CFG_parseFileAtPath(struct CFG_ParseObj *rootObj, const char *path)
{
   struct CFG_Context ctx;
   void *data = NULL;

   memset(&ctx, 0, sizeof(ctx));
   ctx.allow_buffering = 1;
   if( !(ctx.file = fopen(path, "r")) ){
      ERRNO_WARN("Failed to fdopen config file '%s'", path);
      return NULL;
    }

   if (rootObj && rootObj->initCb.cb) {
      data = (*rootObj->initCb.cb)("", "", data,
            rootObj->initCb.p1, rootObj->initCb.p2);

      while(NextLine(&ctx)) {
         if (ctx.buff[0] == '<') {
            data = ParseObject(&ctx, rootObj, data);
            continue;
         }
         data = processKeyValueLine(&ctx, rootObj->keys, data);
      }
   }

   ERR_WARN(fclose(ctx.file),
         "Failed to close config file '%s'", path);

   if (rootObj && rootObj->finiCb.cb)
      data = (*rootObj->finiCb.cb)("", "", data,
            rootObj->finiCb.p1, rootObj->finiCb.p2);

   return data;
}

void *CFG_MallocCB(const char *key, const char *value, void *data, void *p1,
            void *p2)
{
   size_t sz = (size_t)p1;
   void *res = malloc(sz);
   if(!res){
      DBG_print(DBG_LEVEL_WARN,"malloc error");
      return NULL;
   }
   memset(res, 0, sz);

   return res;
}

void *CFG_StrdupCB(const char *key, const char *value, void *data, void *p1,
            void *p2)
{
   void *dest_v = data + (p1 - ((void*)0));
   char ** dest = (char**)dest_v;

   *dest = strdup(value);
   if(!*dest){
      ERRNO_WARN("strdup error:");
      return NULL;
   }

   return data;
}

void *CFG_uint32_CB(const char *key, const char *value, void *data, void *p1,
            void *p2)
{
   void *dest_v = data + (p1 - ((void*)0));
   uint32_t *dest = (uint32_t*)dest_v;

   errno = 0;
   *dest = strtol(value, NULL, 0);
   if(errno){
      ERRNO_WARN("strtol error:");
      return NULL;
   }

   return data;
}

void *CFG_uint16_CB(const char *key, const char *value, void *data, void *p1,
            void *p2)
{
   void *dest_v = data + (p1 - ((void*)0));
   uint16_t *dest = (uint16_t*)dest_v;

   errno = 0;
   *dest = strtol(value, NULL, 0);
   if(errno){
      ERRNO_WARN("strtol error:");
      return NULL;
   }

   return data;
}

void *CFG_int32_CB(const char *key, const char *value, void *data, void *p1,
            void *p2)
{
   void *dest_v = data + (p1 - ((void*)0));
   int32_t *dest = (int32_t*)dest_v;

   errno = 0;
   *dest = strtol(value, NULL, 0);
   if(errno){
      ERRNO_WARN("strtol error:");
      return NULL;
   }

   return data;
}

void *CFG_int16_CB(const char *key, const char *value, void *data, void *p1,
            void *p2)
{
   void *dest_v = data + (p1 - ((void*)0));
   int16_t *dest = (int16_t*)dest_v;

   errno = 0;
   *dest = strtol(value, NULL, 0);
   if(errno){
      ERRNO_WARN("strtol error:");
      return NULL;
   }

   return data;
}

void *CFG_float_CB(const char *key, const char *value, void *data, void *p1,
            void *p2)
{
   void *dest_v = data + (p1 - ((void*)0));
   float *dest = (float*)dest_v;

   errno = 0;
   *dest = strtof(value, NULL);
   if(errno){
      ERRNO_WARN("strtof error:");
      return NULL;
   }

   return data;
}

void *CFG_InetAtonCB(const char *key, const char *value, void *data, void *p1,
            void *p2)
{
   void *dest_v = data + (p1 - ((void*)0));
   struct in_addr *dest = (struct in_addr*)dest_v;

   if( !inet_aton(value, dest) ){
      DBG_print(DBG_LEVEL_WARN,"value not converted correctly");
   }
   // ERRNO_WARN("inet_aton error");

   return data;
}

void *CFG_InetDNSCB(const char *key, const char *value, void *data, void *p1,
            void *p2)
{
   void *dest_v = data + (p1 - ((void*)0));
   struct in_addr *dest = (struct in_addr*)dest_v;
   struct addrinfo hint;
   struct addrinfo *res = NULL, *curr;

   memset(&hint, 0, sizeof(hint));
   hint.ai_family = AF_INET;

   if (!getaddrinfo(value, NULL, &hint, &res)) {
      for (curr = res; curr; curr = curr->ai_next) {
         if (curr->ai_family == AF_INET) {
            *dest = ((struct sockaddr_in*)curr->ai_addr)->sin_addr;
            break;
         }
      }

      if (res)
         freeaddrinfo(res);
   }
   else
      DBG_print(DBG_LEVEL_WARN, "Failed to resolve hostname %s: %s", value,
            strerror(errno));

   return data;
}

void *CFG_PtrCpyCB(const char *key, const char *value, void *data, void *p1,
            void *p2)
{
   void *dest_v = data + (p1 - ((void*)0));
   void **dest = (void**)dest_v;

   *dest = (void*)value;

   return data;
}

void *CFG_PtrArrayAppendCB(const char *key, const char *value, void *data, void *p1,
            void *p2)
{
   void *dest_v = data + (p1 - ((void*)0));
   struct CFG_Array *dest = (struct CFG_Array *)dest_v;
   void *newData;

   if (dest->len == dest->capacity) {
      dest->capacity += 5;
      newData = malloc(sizeof(void*) * dest->capacity);
      if(!newData){
        ERRNO_WARN("malloc error:");
        return NULL;
      }
      if (dest->data) {
         memcpy(newData, dest->data, sizeof(void*) * dest->len);
         free(dest->data);
      }
      dest->data = newData;
   }

   dest->data[dest->len++] = (void*)value;

   return data;
}

void CFG_freeArray(struct CFG_Array *arr, CFG_objFreeCb_t freeCb)
{
   if (arr && arr->data) {
      for(int i = 0; freeCb && i < arr->len; i++)
         (*freeCb)(arr->data[i]);

      free(arr->data);
      arr->data = NULL;
      arr->capacity = 0;
      arr->len = 0;
   }
}

struct CFG_ParseObj *CFG_static_obj(
      const char *params, void *arg, int *buffer_obj)
{
   *buffer_obj = 0;
   return (struct CFG_ParseObj*)arg;
}

struct CFG_ParseObj *CFG_static_arr_obj(
      const char *params, void *arg, int *buffer_obj)
{
   *buffer_obj = 0;
   struct CFG_ParseObj *result = NULL, **itr;

   itr = (struct CFG_ParseObj**)arg;

   while(*itr) {
      if (!strcasecmp((*itr)->subtype, params)) {
         result = *itr;
         break;
      }
      itr++;
   }

   return result;
}
