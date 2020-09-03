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
 * Header file for config file management.
 * @author John Bellardo
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*CFG_parseCB_t)(const char *key, const char *value, void *data,
      void *p1, void *p2);

struct CFG_ParseObj;
struct CFG_buffered_obj;
typedef struct CFG_ParseObj *(*CFG_objlookup_cb_t)(const char *name,
      void *arg, int *buffer_obj);

struct CFG_ParseCB
{
   CFG_parseCB_t cb;
   void *p1, *p2;
};


struct CFG_ParseValue
{
   const char *name;
   CFG_objlookup_cb_t lookup_cb;
   void *lookup_arg;
   struct CFG_ParseCB cb;
};

struct CFG_ParseObj
{
   const char *subtype;
   struct CFG_ParseCB initCb, finiCb;
   struct CFG_ParseValue *keys;
};

struct CFG_Array {
   int len, capacity;
   void **data;
};

void *CFG_MallocCB(const char *key, const char *value, void *data, void *p1,
      void *p2);
void *CFG_StrdupCB(const char *key, const char *value, void *data, void *p1,
      void *p2);
void *CFG_InetAtonCB(const char *key, const char *value, void *data, void *p1,
      void *p2);
void *CFG_InetDNSCB(const char *key, const char *value, void *data, void *p1,
      void *p2);
void *CFG_uint32_CB(const char *key, const char *value, void *data, void *p1,
      void *p2);
void *CFG_uint16_CB(const char *key, const char *value, void *data, void *p1,
      void *p2);
void *CFG_int32_CB(const char *key, const char *value, void *data, void *p1,
      void *p2);
void *CFG_int16_CB(const char *key, const char *value, void *data, void *p1,
      void *p2);
void *CFG_float_CB(const char *key, const char *value, void *data, void *p1,
      void *p2);
void *CFG_PtrCpyCB(const char *key, const char *value, void *data, void *p1,
      void *p2);
void *CFG_PtrArrayAppendCB(const char *key, const char *value, void *data,
      void *p1, void *p2);

#define CFG_MALLOC(x) { &CFG_MallocCB, (void*)(sizeof(x)), NULL }
#define CFG_NULL { NULL, NULL, NULL }
#define CFG_NULLK { NULL, NULL, NULL, CFG_NULL }
#define CFG_OBJ(n, obj, str, field) { n, &CFG_static_obj, obj, { &CFG_PtrCpyCB, (void*)offsetof(str,field), NULL } }
#define CFG_OBJLIST(n, objlist, str, field) { n, &CFG_static_arr_obj, objlist, { &CFG_PtrCpyCB, (void*)offsetof(str,field), NULL } }
#define CFG_OBJARR(n, obj, str, field) { n, &CFG_static_obj, obj, { &CFG_PtrArrayAppendCB, (void*)offsetof(str,field), NULL } }

#define CFG_OBJLISTARR(n, objlist, str, field) { n, &CFG_static_arr_obj, objlist, { &CFG_PtrArrayAppendCB, (void*)offsetof(str,field), NULL } }

#define CFG_OBJCB(n, obj, cb) { n, &CFG_static_obj, obj, { &cb, NULL, NULL } }
#define CFG_STRDUP(n, str, field) { n, NULL, NULL, { &CFG_StrdupCB, (void*)offsetof(str,field), NULL } }
#define CFG_UINT32(n, str, field) { n, NULL, NULL, { &CFG_uint32_CB, (void*)offsetof(str,field), NULL } }
#define CFG_UINT16(n, str, field) { n, NULL, NULL, { &CFG_uint16_CB, (void*)offsetof(str,field), NULL } }
#define CFG_INT32(n, str, field) { n, NULL, NULL, { &CFG_int32_CB, (void*)offsetof(str,field), NULL } }
#define CFG_INT16(n, str, field) { n, NULL, NULL, { &CFG_int16_CB, (void*)offsetof(str,field), NULL } }
#define CFG_FLOAT(n, str, field) { n, NULL, NULL, { &CFG_float_CB, (void*)offsetof(str,field), NULL } }
#define CFG_INET_ATON(n, str, field) { n, NULL, NULL, { &CFG_InetAtonCB, (void*)offsetof(str,field), NULL } }
#define CFG_INET_DNS(n, str, field) { n, NULL, NULL, { &CFG_InetDNSCB, (void*)offsetof(str,field), NULL } }

#define CFG_OBJNAME(x) CFG__##x##__obj
#define CFG_VALNAME(x) CFG__##x##__keys
#define CFG_SUBNAME(x) CFG__##x##__subtypes
#define CFG_VALNAME_CPP(c,x) CFG__##c##_##x##__keys

#define CFG_NEWOBJ_GBL(x, ini, fin, ...) static struct CFG_ParseValue CFG_VALNAME(x)[] = { __VA_ARGS__, CFG_NULLK }; \
struct CFG_ParseObj CFG_OBJNAME(x) = { "", ini, fin, CFG_VALNAME(x) };
#define CFG_NEWOBJ(x, ini, fin, ...) static struct CFG_ParseValue CFG_VALNAME(x)[] = { __VA_ARGS__, CFG_NULLK }; \
static struct CFG_ParseObj CFG_OBJNAME(x) = { "", ini, fin, CFG_VALNAME(x) };
#define CFG_NEWSUBOBJ(x, name, ini, fin, ...) static struct CFG_ParseValue CFG_VALNAME(x)[] = { __VA_ARGS__, CFG_NULLK }; \
struct CFG_ParseObj CFG_OBJNAME(x) = { name, ini, fin, CFG_VALNAME(x) };

#define CFG_EMPTYOBJ(x, ini, fin) static struct CFG_ParseValue CFG_VALNAME(x)[] = { CFG_NULLK }; \
static struct CFG_ParseObj CFG_OBJNAME(x) = { "", ini, fin, CFG_VALNAME(x) };

#define CFG_OBJ_CPPDECL(x) static struct CFG_ParseObj x
#define CFG_NEWOBJ_CPP(c, x, ini, fin, ...) static struct CFG_ParseValue CFG_VALNAME_CPP(x,c)[] = { __VA_ARGS__, CFG_NULLK }; \
struct CFG_ParseObj c::x = { "", ini, fin, CFG_VALNAME_CPP(x,c) };
#define CFG_NEWSUBOBJ_CPP(c, x, name, ini, fin, ...) static struct CFG_ParseValue CFG_VALNAME_CPP(c,x)[] = { __VA_ARGS__, CFG_NULLK }; \
struct CFG_ParseObj c::x = { name, ini, fin, CFG_VALNAME_CPP(c,x) };
/**
 * \brief Get the path of the configuration file.
 * \returns The full POSIX path of the current configuration file.  If there is
 * no file the string is empty and the pointer is non-NULL.
 **/
const char *CFG_getPath();

/**
 * \brief Locates the configuration file with the given name.  It searches in
 *  the current working directory, in $HOME/.<filename>, and in /etc/<filename>
 *  in that order.
 * \returns True if a configuration file is located and false otherwise.
 **/
int CFG_locateConfigFile(const char *name);

/**
 * \brief This function parses the configuration file
 **/
void *CFG_parseFile(struct CFG_ParseObj *);

/**
 * \brief This function parses the configuration file located at the
 * given path.  This bypassed the normal directory search order.
 **/
void *CFG_parseFileAtPath(struct CFG_ParseObj *, const char *path);

/// Prototype for a function that knows how to clean up memory for a struct
typedef void (*CFG_objFreeCb_t)(void*);

/**
 * \brief Free any heap memory associated with an array structure
 **/
void CFG_freeArray(struct CFG_Array *arr, CFG_objFreeCb_t freeCb);

/**
  * \brief Object description function that always returns the pointer 
  *           from the lookup_arg field.
  **/
struct CFG_ParseObj *CFG_static_obj(
      const char *name, void *arg, int *buffer_obj);

/**
  * \brief Object description function that loops through an array of static
  *           object descriptions and matches based on name.
  **/
struct CFG_ParseObj *CFG_static_arr_obj(
      const char *name, void *arg, int *buffer_obj);

void *CFG_cfg_for_object_buffer(struct CFG_buffered_obj *obj);
#ifdef __cplusplus
}
#endif

#endif
