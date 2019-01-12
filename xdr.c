#include <arpa/inet.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "xdr.h"
#include "events.h"
#include "hashtable.h"
#include <inttypes.h>

#define ASCII2HEX(c) ( ( (c) >= '0' && (c) <= '9' ? (c) - '0' : \
      ((c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 10 : \
      ((c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 10 : 0 ))) & 0xF)

static struct HashTable *structHash = NULL;

static size_t xdr_struct_hash_func(void *key)
{
   return (uintptr_t)key;
}

static void *xdr_struct_key_for_data(void *data)
{
   if (!data)
      return 0;
   return (void*)(uintptr_t)(((struct XDR_StructDefinition*)data)->type);
}

static int xdr_struct_cmp_key(void *key1, void *key2)
{
   if ( ((uintptr_t)key1) == ((uintptr_t)key2) )
      return 1;
   return 0;
}

void XDR_register_struct(struct XDR_StructDefinition *def)
{
   if (!def)
      return;

   if (!structHash) {
      structHash = HASH_create_table(37, &xdr_struct_hash_func,
            &xdr_struct_cmp_key, &xdr_struct_key_for_data);
      if (!structHash)
         return;
   }

   HASH_add_data(structHash, def);
}

void XDR_register_structs(struct XDR_StructDefinition *structs)
{
   if (!structs)
      return;

   while (structs->type && structs->encoder && structs->decoder) {
      XDR_register_struct(structs);
      structs++;
   }
}

void XDR_register_populator(XDR_populate_struct cb, void *arg, uint32_t type)
{
   struct XDR_StructDefinition *def = NULL;

   def = XDR_definition_for_type(type);
   if (!def)
      return;

   def->populate = cb;
   def->populate_arg = arg;
}

int XDR_decode_byte_array(char *src, char **dst, size_t *used,
      size_t max, void *lenptr)
{
   int32_t byte_len;
   int padding;

   memcpy(&byte_len, lenptr, sizeof(byte_len));
   padding = (4 - (byte_len % 4)) % 4;
   *used = 0;
   if (!dst || byte_len + padding >= max)
      return -1;
   *used = byte_len + padding;

   *dst = malloc(byte_len);
   memcpy(*dst, src, byte_len);

   return 0;
}

int XDR_encode_byte_array(char **src, char *dst, size_t *used, size_t max,
      void *lenptr)
{
   int32_t byte_len;
   int padding;

   byte_len = *(int32_t*)lenptr;
   padding = (4 - (byte_len % 4)) % 4;
   *used = byte_len + padding;
   if (!dst || !src || !*src || byte_len + padding >= max)
      return -1;

   memcpy(dst, *src, byte_len);
   if (padding)
      memset(dst + byte_len, 0, padding);

   return 0;
}

int XDR_decode_int32_array(char *src, int32_t **dst,
      size_t *used, size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(uint32_t),
            (XDR_Decoder)&XDR_decode_uint32, NULL);

   return 0;
}

int XDR_decode_int32(char *src, int32_t *dst, size_t *inc, size_t max,
      void *len)
{
   int32_t net;
   if (max < sizeof(*dst))
      return -1;

   memcpy(&net, src, sizeof(net));
   *dst = (int32_t)ntohl(net);
   *inc = sizeof(net);
   return 0;
}

int XDR_decode_uint32_array(char *src, uint32_t **dst,
      size_t *used, size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(uint32_t),
            (XDR_Decoder)&XDR_decode_uint32, NULL);

   return 0;
}

int XDR_decode_uint32(char *src, uint32_t *dst, size_t *inc, size_t max,
      void *len)
{
   uint32_t net;
   if (max < sizeof(*dst))
      return -1;

   memcpy(&net, src, sizeof(net));
   *dst = ntohl(net);
   *inc = sizeof(net);
   return 0;
}

int XDR_decode_int64_array(char *src, int64_t **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(int64_t),
            (XDR_Decoder)&XDR_decode_int64, NULL);

   return 0;
}

int XDR_decode_int64(char *src, int64_t *dst, size_t *inc, size_t max,
      void *len)
{
   int32_t hi;
   uint32_t low;
   int64_t res;
   size_t used = 0;

   if (max < sizeof(*dst))
      return -1;

   if (XDR_decode_int32(src, &hi, &used, max, NULL) < 0)
      return -1;
   src += used;
   max -= used;
   if (XDR_decode_uint32(src, &low, &used, max, NULL) < 0)
      return -1;

   res = hi;
   res <<= 32;
   *dst = res | low;
   *inc = sizeof(*dst);

   return 0;
}

int XDR_decode_uint64_array(char *src, uint64_t **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(uint64_t),
            (XDR_Decoder)&XDR_decode_uint64, NULL);

   return 0;
}

int XDR_decode_uint64(char *src, uint64_t *dst, size_t *inc, size_t max,
      void *len)
{
   uint32_t hi;
   uint32_t low;
   uint64_t res;
   size_t used = 0;

   if (max < sizeof(*dst))
      return -1;

   if (XDR_decode_uint32(src, &hi, &used, max, NULL) < 0)
      return -1;
   src += used;
   max -= used;
   if (XDR_decode_uint32(src, &low, &used, max, NULL) < 0)
      return -1;

   res = hi;
   res <<= 32;
   *dst = res | low;
   *inc = sizeof(*dst);

   return 0;
}

int XDR_decode_float_array(char *src, float **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(float),
            (XDR_Decoder)&XDR_decode_float, NULL);

   return 0;
}

int XDR_decode_float(char *src, float *dst, size_t *inc, size_t max,
      void *len)
{
   if (max < sizeof(*dst))
      return -1;

   memcpy(dst, src, sizeof(*dst));
   *inc = sizeof(*dst);

   return 0;
}

int XDR_encode_float_array(float **src, char *dst,
      size_t *used, size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(float),
            (XDR_Encoder)&XDR_encode_float, NULL);

   return 0;
}

int XDR_encode_float(float *src, char *dst, size_t *used, size_t max,
      void *len)
{
   *used = sizeof(*src);
   if (max < *used)
      return -1;
   memcpy(dst, src, *used);
   return 0;
}

int XDR_encode_double_array(double **src, char *dst,
      size_t *used, size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(double),
            (XDR_Encoder)&XDR_encode_float, NULL);

   return 0;
}

int XDR_encode_double(double *src, char *dst, size_t *used, size_t max,
      void *len)
{
   *used = sizeof(*src);
   if (max < *used)
      return -1;
   memcpy(dst, src, *used);
   return 0;
}

int XDR_decode_double_array(char *src, double **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(double),
            (XDR_Decoder)&XDR_decode_double, NULL);

   return 0;
}

int XDR_decode_double(char *src, double *dst, size_t *inc, size_t max,
      void *len)
{
   if (max < sizeof(*dst))
      return -1;

   memcpy(dst, src, sizeof(*dst));
   *inc = sizeof(*dst);

   return 0;
}

int XDR_decode_union_array(char *src, struct XDR_Union **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct XDR_Union),
            (XDR_Decoder)&XDR_decode_union, NULL);

   return 0;
}

int XDR_decode_union(char *src, struct XDR_Union *dst, size_t *inc, size_t max,
      void *len)
{
   size_t used;
   struct XDR_StructDefinition *def = NULL;

   *inc = 0;
   dst->data = NULL;
   if (XDR_decode_uint32(src, &dst->type, &used, max, NULL) < 0)
      return -1;

   def = XDR_definition_for_type(dst->type);
   if (!def || !def->decoder)
      return -1;

   *inc = used;
   max -= used;
   src += used;

   dst->data = def->allocator(def);
   if (!dst->data)
      return -1;

   used = 0;
   if (def->decoder(src, dst->data, &used, max, def->arg) < 0) {
      def->deallocator(&dst->data, def);
      dst->data = NULL;
      return -1;
   }
   *inc += used;
   return 0;
} 

int XDR_decode_string(char *src, char **dst,
      size_t *used, size_t max, void *len)
{
   // String arrays are not supported
   assert(0);
}

int XDR_decode_string_array(char *src, char **dst, size_t *inc,
      size_t max, void *len)
{
   size_t used;
   char *str;
   uint32_t str_len, padding;

   *inc = 0;
   used = 0;
   if (XDR_decode_uint32(src, &str_len, &used, max, NULL) < 0)
      return -1;

   *inc = used;
   padding = (4 - (str_len % 4)) % 4;
   if (used + str_len + padding > max)
      return -1;

   str = malloc(str_len + 1);
   memcpy(str, src + used, str_len);
   str[str_len] = 0;
   *dst = str;
   *inc += str_len + padding;

   return 0;
}

int XDR_encode_string(const char *src, char *dst,
      size_t *used, size_t max, void *len)
{
   // Strings outside arrays are not supported
   assert(0);
}

int XDR_encode_string_array(const char **src_ptr, char *dst, size_t *used,
      size_t max, void *len)
{
   uint32_t str_len = 0, padding;
   int res;
   const char *src = NULL;

   *used = 0;
   if (src_ptr && *src_ptr) {
      src = *src_ptr;
      str_len = strlen(src);
   }
   padding = (4 - (str_len % 4)) % 4;

   res = XDR_encode_uint32(&str_len, dst, used, max, NULL);
   dst += *used;
   *used += str_len + padding;
   if (res < 0)
      return res;
   if (max < *used)
      return -2;

   if (src)
      memcpy(dst, src, str_len);
   if (padding)
      memset(dst + str_len, 0, padding);

   return 0;
}

int XDR_encode_uint32_array(uint32_t **src, char *dst,
      size_t *used, size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(uint32_t),
            (XDR_Encoder)&XDR_encode_uint32, NULL);

   return 0;
}

int XDR_encode_uint32(uint32_t *src, char *dst, size_t *used, size_t max,
      void *len)
{
   uint32_t net;
   *used = sizeof(net);

   if (!dst)
      return 0;
   if (!src)
      return -1;
   if (max < sizeof(*src))
      return -2;

   net = htonl(*src);
   memcpy(dst, &net, sizeof(net));

   return 0;
}

int XDR_encode_int32_array(int32_t **src, char *dst,
      size_t *used, size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(int32_t),
            (XDR_Encoder)&XDR_encode_int32, NULL);

   return 0;
}

int XDR_encode_int32(int32_t *src, char *dst, size_t *used, size_t max,
      void *len)
{
   uint32_t net;
   *used = sizeof(net);

   if (!dst)
      return 0;
   if (!src)
      return -1;
   if (max < sizeof(*src))
      return -2;

   net = htonl(*src);
   memcpy(dst, &net, sizeof(net));

   return 0;
}

int XDR_encode_int64_array(int64_t **src, char *dst,
      size_t *used, size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(int64_t),
            (XDR_Encoder)&XDR_encode_int64, NULL);

   return 0;
}

int XDR_encode_int64(int64_t *src, char *dst, size_t *used, size_t max,
      void *unused)
{
   int32_t hi;
   uint32_t low;
   size_t len = 0;

   *used = sizeof(*src);
   if (!dst)
      return 0;
   if (!src)
      return -1;
   if (max < sizeof(*src))
      return -1;

   hi = (*src) >> 32;
   low = (*src) & 0xFFFFFFFF;

   if (XDR_encode_int32(&hi, dst, &len, max, NULL) < 0)
      return -1;

   dst += len;
   max -= len;

   if (XDR_encode_uint32(&low, dst, &len, max, NULL) < 0)
      return -1;

   return 0;
}

int XDR_encode_uint64_array(uint64_t **src, char *dst,
      size_t *used, size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(uint64_t),
            (XDR_Encoder)&XDR_encode_uint64, NULL);

   return 0;
}

int XDR_encode_uint64(uint64_t *src, char *dst, size_t *used, size_t max,
      void *unused)
{
   uint32_t hi;
   uint32_t low;
   size_t len = 0;

   *used = sizeof(*src);
   if (!dst)
      return 0;
   if (!src)
      return -1;
   if (max < sizeof(*src))
      return -1;

   hi = (*src) >> 32;
   low = (*src) & 0xFFFFFFFF;

   if (XDR_encode_uint32(&hi, dst, &len, max, NULL) < 0)
      return -1;

   dst += len;
   max -= len;

   if (XDR_encode_uint32(&low, dst, &len, max, NULL) < 0)
      return -1;

   return 0;
}

int XDR_encode_union_array(struct XDR_Union **src, char *dst,
      size_t *used, size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct XDR_Union),
            (XDR_Encoder)&XDR_encode_union, NULL);

   return 0;
}

int XDR_encode_union(struct XDR_Union *src, char *dst,
      size_t *inc, size_t max, void *unused)
{
   struct XDR_StructDefinition *def = NULL;
   size_t used = 0;
   int res, res2;

   res = XDR_encode_uint32(&src->type, dst, &used, max, NULL);
   *inc = used;

   def = XDR_definition_for_type(src->type);
   if (!def || !def->encoder)
      return -1;
   
   if (dst && res >= 0) {
      dst += used;
      max -= used;
   }
   else
      dst = NULL;

   used = 0;
   res2 = def->encoder(src->data, dst, &used, max, def->type, def->arg);
   *inc += used;
   if (res < 0 || res2 < 0)
      return -1;

   return 0;
}

int XDR_struct_decoder(char *src, void *dst_void, size_t *inc,
      size_t max, void *arg)
{
   size_t used = 0, len = 0;
   struct XDR_FieldDefinition *field = arg;
   char *dst = (char*)dst_void;

   if (!field)
      return -1;

   while (field->offset || field->decoder || field->encoder) {
      if (field->decoder(src + used, dst + field->offset, &len,
               max - used, dst + field->len_offset) < 0)
         return -1;
      used += len;
      len = 0;
      field++;
   }

   *inc = used;

   return 0;
}

int XDR_struct_encoder(void *src_void, char *dst, size_t *inc,
      size_t max, uint32_t type, void *arg)
{
   size_t len = 0;
   struct XDR_FieldDefinition *field = arg;
   char *src = (char*)src_void;
   size_t used = 0;
   int res = 0;

   *inc = 0;

   if (!field)
      return 0;

   while (field->offset || field->decoder || field->encoder) {
      if (!dst || res < 0)
         field->encoder(src + field->offset, NULL, &len, max,
               src + field->len_offset);
      else
         res = field->encoder(src + field->offset, dst + used, &len, max-used,
               src + field->len_offset);
      used += len;
      field++;
   }

   *inc = used;

   return res;
}

void *XDR_malloc_allocator(struct XDR_StructDefinition *def)
{
   void *result;

   if (!def || !def->in_memory_size)
      return NULL;

   result = malloc(def->in_memory_size);
   if (result)
      memset(result, 0, def->in_memory_size);

   return result;
}

void XDR_struct_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field)
{
   struct XDR_StructDefinition *def;

   if (!goner || !field)
      return;
   def = XDR_definition_for_type(field->struct_id);
   if (!def || !def->deallocator)
      return;

   def->deallocator(goner, def);
}

void XDR_struct_array_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field)
{
   // Needs to be written!!
   assert(0);
}

void XDR_union_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field)
{
   struct XDR_StructDefinition *def;
   struct XDR_Union *u;

   if (!goner)
      return;
   u = (struct XDR_Union*)*(struct XDR_Union**)goner;
   def = XDR_definition_for_type(u->type);
   if (def && def->deallocator)
      def->deallocator(&u->data, def);
   else
      free(u->data);
}

void XDR_array_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field)
{
   if (!goner || !*goner)
      return;

   free(*goner);
}

void XDR_free_deallocator(void **goner, struct XDR_StructDefinition *def)
{
   void *to_free;

   if (!goner || !*goner)
      return;

   to_free = *goner;
   *goner = NULL;
   free(to_free);
}

void XDR_struct_free_deallocator(void **goner, struct XDR_StructDefinition *def)
{
   struct XDR_FieldDefinition *fields;
   char *to_free;

   if (!goner || !*goner || !def)
      return;
   fields = (struct XDR_FieldDefinition*)def->arg;
   to_free = (char*)*goner;

   for (; fields && fields->decoder && fields->encoder; fields++) {
      if (!fields->dealloc)
         continue;
      fields->dealloc( (void**)(to_free + fields->offset), fields);
   }

   *goner = NULL;
   free(to_free);
}

void XDR_print_field_double(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *unused)
{
   double val;

   if (!data)
      return;
   memcpy(&val, data, sizeof(val));

   if (style == XDR_PRINT_HUMAN && 0 != field->conv_divisor)
      fprintf(out, "%lf", val/field->conv_divisor + field->conv_offset);
   else
      fprintf(out, "%lf", val);
}

void XDR_print_field_float(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *unused)
{
   float val;

   if (!data)
      return;
   memcpy(&val, data, sizeof(val));

   if (style == XDR_PRINT_HUMAN && 0 != field->conv_divisor)
      fprintf(out, "%lf", val/field->conv_divisor + field->conv_offset);
   else
      fprintf(out, "%f", val);
}

void XDR_print_field_float_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len)
{
   XDR_array_field_printer(out, data, field, style, len, 
         &XDR_print_field_float, sizeof(float));
}

void XDR_print_field_char(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *unused)
{
   int32_t val;

   if (!data)
      return;
   memcpy(&val, data, sizeof(val));

   fprintf(out, "%c", val);
}

void XDR_print_field_char_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len)
{
   XDR_array_field_printer(out, data, field, style, len, 
         &XDR_print_field_char, sizeof(int32_t));
}

void XDR_print_field_int32(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *unused)
{
   int32_t val;

   if (!data)
      return;
   memcpy(&val, data, sizeof(val));

   if (style == XDR_PRINT_HUMAN && 0 != field->conv_divisor)
      fprintf(out, "%lf", val/field->conv_divisor + field->conv_offset);
   else
      fprintf(out, "%d", val);
}

void XDR_print_field_int32_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len)
{
   XDR_array_field_printer(out, data, field, style, len, 
         &XDR_print_field_int32, sizeof(int32_t));
}

void XDR_print_field_byte_array(FILE *out, void *data_ptr,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len)
{
   unsigned char *data;
   int i;

   if (!len || !data_ptr)
      return;
   data = *(unsigned char**)data_ptr;
   if (!data)
      return;

   for (i = 0; i < *(int*)len; i++)
      fprintf(out, "%02X", data[i]);
}

void XDR_print_field_string_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *unused)
{
   char **str = (char **)data;

   if (*str)
      fprintf(out, "%s", *str);
}

void XDR_print_field_string(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len)
{
   // Grammar doesn't support a string outside an array
   assert(0);
}

void XDR_print_field_uint32(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *unused)
{
   uint32_t val;

   if (!data)
      return;
   memcpy(&val, data, sizeof(val));

   if (style == XDR_PRINT_HUMAN && 0 != field->conv_divisor)
      fprintf(out, "%lf", val/field->conv_divisor + field->conv_offset);
   else
      fprintf(out, "%u", val);
}

void XDR_print_field_uint32_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len)
{
   XDR_array_field_printer(out, data, field, style, len, 
         &XDR_print_field_uint32, sizeof(uint32_t));
}

void XDR_print_field_int64(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *unused)
{
   int64_t val;

   if (!data)
      return;
   memcpy(&val, data, sizeof(val));

   if (style == XDR_PRINT_HUMAN && 0 != field->conv_divisor)
      fprintf(out, "%lf", val/field->conv_divisor + field->conv_offset);
   else
      fprintf(out, "%"PRId64, val);
}

void XDR_print_field_int64_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len)
{
   XDR_array_field_printer(out, data, field, style, len, 
         &XDR_print_field_int64, sizeof(int64_t));
}

void XDR_print_field_uint64(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *unused)
{
   uint64_t val;

   if (!data)
      return;
   memcpy(&val, data, sizeof(val));

   if (style == XDR_PRINT_HUMAN && 0 != field->conv_divisor)
      fprintf(out, "%lf", val/field->conv_divisor + field->conv_offset);
   else
      fprintf(out, "%"PRIu64, val);
}

void XDR_print_field_uint64_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len)
{
   XDR_array_field_printer(out, data, field, style, len, 
         &XDR_print_field_uint64, sizeof(uint64_t));
}

void XDR_print_field_union(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *unused)
{
   struct XDR_Union val;
   struct XDR_StructDefinition *def = NULL;

   if (!data)
      return;
   memcpy(&val, data, sizeof(val));

   def = XDR_definition_for_type(val.type);
   if (def && def->print_func)
      def->print_func(out, val.data, def->arg, style);
}

void XDR_print_field_union_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len)
{
   XDR_array_field_printer(out, data, field, style, len, 
         &XDR_print_field_union, sizeof(struct XDR_Union));
}

void XDR_scan_float(const char *in, void *dst, void *arg, void *unused)
{
   float val;
   sscanf(in, "%f", &val);
   memcpy(dst, &val, sizeof(val));
}

void XDR_scan_float_array(const char *in, void *dst, void *arg, void *len)
{
   XDR_array_field_scanner(in, dst, arg, len, &XDR_scan_float,
      arg, sizeof(float));
}

void XDR_scan_double(const char *in, void *dst, void *arg, void *unused)
{
   double val;
   sscanf(in, "%lf", &val);
   memcpy(dst, &val, sizeof(val));
}

void XDR_scan_double_array(const char *in, void *dst, void *arg, void *len)
{
   XDR_array_field_scanner(in, dst, arg, len, &XDR_scan_double,
      arg, sizeof(double));
}

void XDR_scan_int32(const char *in, void *dst, void *arg, void *unused)
{
   int32_t val;
   sscanf(in, "%d", &val);
   memcpy(dst, &val, sizeof(val));
}

void XDR_scan_int32_array(const char *in, void *dst, void *arg,
      void *len)
{
   XDR_array_field_scanner(in, dst, arg, len, &XDR_scan_int32,
      arg, sizeof(int32_t));
}

void XDR_scan_char(const char *in, void *dst, void *arg, void *unused)
{
   int32_t val;
   char c;
   sscanf(in, "%c", &c);
   val = c;
   memcpy(dst, &val, sizeof(val));
}

void XDR_scan_char_array(const char *in, void *dst, void *arg,
      void *len)
{
   XDR_array_field_scanner(in, dst, arg, len, &XDR_scan_char,
      arg, sizeof(int32_t));
}

void XDR_scan_uint32(const char *in, void *dst, void *arg, void *unused)
{
   uint32_t val;
   sscanf(in, "%u", &val);
   memcpy(dst, &val, sizeof(val));
}

void XDR_scan_uint32_array(const char *in, void *dst, void *arg,
      void *len)
{
   XDR_array_field_scanner(in, dst, arg, len, &XDR_scan_uint32,
      arg, sizeof(uint32_t));
}

void XDR_scan_int64(const char *in, void *dst, void *arg, void *unused)
{
   int64_t val;
   sscanf(in, "%"SCNd64, &val);
   memcpy(dst, &val, sizeof(val));
}

void XDR_scan_int64_array(const char *in, void *dst, void *arg,
      void *len)
{
   XDR_array_field_scanner(in, dst, arg, len, &XDR_scan_int64,
      arg, sizeof(int64_t));
}

void XDR_scan_uint64(const char *in, void *dst, void *arg, void *unused)
{
   uint64_t val;
   sscanf(in, "%"SCNu64, &val);
   memcpy(dst, &val, sizeof(val));
}

void XDR_scan_uint64_array(const char *in, void *dst, void *arg,
      void *len)
{
   XDR_array_field_scanner(in, dst, arg, len, &XDR_scan_uint64,
      arg, sizeof(uint64_t));
}

void XDR_scan_string(const char *in, void *dst, void *arg, void *unused)
{
   char **str = (char**)dst;

   if (!*str)
      *str = strdup(in);
   else
      strcpy(*str, in);
}

void XDR_scan_string_array(const char *in, void *dst, void *arg,
      void *len)
{
   char **str = (char**)dst;

   if (!*str)
      *str = strdup(in);
   else
      strcpy(*str, in);
}

void XDR_scan_byte(const char *in, void *dst, void *arg, void *len)
{
   if (!in || !in[0] || !dst)
      return;

   *(unsigned char*)dst = 0;
   if (in[0])
      *(unsigned char*)dst = (ASCII2HEX(in[0]) << 4) | ASCII2HEX(in[1]);
}

void XDR_scan_byte_array(const char *in, void *dst_ptr, void *arg, void *len_ptr)
{
   unsigned char *dst = NULL;
   int i = 0, *len;

   if (!dst_ptr || !len_ptr)
      return;
   len = (int*)len_ptr;
   *len = (strlen(in) + 1) / 2;

   dst = malloc(*len);
   *(unsigned char**)dst_ptr = dst;

   for (i = 0; i < *len; i++, in += 2)
      dst[i] = (ASCII2HEX(in[0]) << 4) | ASCII2HEX(in[1]);
}

void XDR_print_structure(uint32_t type, struct XDR_StructDefinition *str,
      char *buff, size_t len, void *arg, int arg2)
{
   void *data; 
   size_t used = 0;
   enum XDR_PRINT_STYLE style = (enum XDR_PRINT_STYLE)arg2;

   if (!str || !str->print_func || !str->allocator || !str->deallocator)
      return;

   data = str->allocator(str);
   if (!data)
      return;

   if (str->decoder(buff, data, &used, len, str->arg) >= 0)
      str->print_func((FILE*)arg, data, str->arg, style);

   str->deallocator(&data, str);
}

void XDR_print_fields_func(FILE *out, void *data_void, void *arg,
      enum XDR_PRINT_STYLE style)
{
   struct XDR_FieldDefinition *fields = (struct XDR_FieldDefinition*)arg;
   char *data = (char*)data_void;
   const char *name;
   int line = 0;

   for (; fields && fields->decoder && fields->encoder; fields++) {
      if (!fields->printer)
         continue;

      if (style == XDR_PRINT_KVP && fields->key) {
         fprintf(out, "%s=", fields->key);
         fields->printer(out, data + fields->offset, fields, style,
               data + fields->len_offset);
         fprintf(out, "\n");
      }
      if (style == XDR_PRINT_HUMAN && (fields->key || fields->name)) {
         name = fields->name;
         if (!name)
            name = fields->key;

         fprintf(out, "%03d:  %-32s", line++, name);
         fields->printer(out, data + fields->offset, fields, style,
               data + fields->len_offset);
         if (fields->unit)
            fprintf(out, "    [%s]\n", fields->unit);
         else
            fprintf(out, "\n");
      }
      if (style == XDR_PRINT_CSV_HEADER && fields->key && fields->printer) {
         fprintf(out, "%s,", fields->key);
      }
      if (style == XDR_PRINT_CSV_DATA && fields->key && fields->printer) {
         fields->printer(out, data + fields->offset, fields, style,
               data + fields->len_offset);
         fprintf(out, ",");
      }
   }
}

struct XDR_StructDefinition *XDR_definition_for_type(uint32_t type)
{
   if (structHash)
      return (struct XDR_StructDefinition *)
         HASH_find_key(structHash, (void*)(uintptr_t)type);
   return NULL;
}

void XDR_free_union(struct XDR_Union *goner)
{
   struct XDR_StructDefinition *def;

   if (!goner || !goner->data)
      return;
   def = XDR_definition_for_type(goner->type);
   if (def && def->deallocator)
      def->deallocator(&goner->data, def);
   else
      free(goner->data);
}

void XDR_array_field_scanner(const char *in, void *dst_ptr, void *arg,
      void *len_ptr,
      XDR_field_scanner scan, void *parg, size_t increment)
{
   char *dst = NULL;
   int i = 0, *len;
   char *sep = NULL, *first;

   if (!dst_ptr || !len_ptr || !scan || !increment || !in)
      return;
   len = (int*)len_ptr;
   if (!*in) {
      *len = 0;
      *(char**)dst_ptr = NULL;
      return;
   }

   *len = 1;
   for (sep = strchr(in, ','); sep; sep = strchr(sep+1, ','))
      (*len)++;

   dst = malloc( (*len) * increment);
   memset(dst, 0, (*len) * increment);
   *(char**)dst_ptr = dst;

   sep = (char*)in - 1;
   do {
      first = sep + 1;
      sep = strchr(first, ',');
      if (sep)
         *sep = 0;
      scan(first, dst + i * increment, parg, NULL);
      if (sep)
         *sep = ',';
      i++;
   } while(sep && i < *len);
}

void XDR_array_field_printer(FILE *out, void *src_ptr,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len_ptr, XDR_print_field_func print, size_t increment)
{
   char *src = NULL;
   int i, len;

   if (!src_ptr || !print || !len_ptr)
      return;
   src = *(char**)src_ptr;
   len = *(int*)len_ptr;
   if (!src)
      return;

   for (i = 0; i < len; i++) {
      if (i > 0)
         fprintf(out, ",");
      print(out, src + i*increment, field, style, NULL);
   }
}

int XDR_array_encoder(char *src_ptr, void *dst, size_t *used, size_t max,
      int len, size_t increment, XDR_Encoder enc, void *enc_arg)
{
   char *src = NULL;
   int i;
   size_t sz = 0, enc_len = 0;
   int res = 0;

   if (src_ptr)
      src = *(char**)src_ptr;

   for (i = 0; i < len; i++) {
      sz = 0;
      if (dst && res >= 0)
         res = enc(src + i*increment, dst + enc_len, &sz, max - enc_len, NULL);
      else
         enc(src + i*increment, NULL, &sz, max - enc_len, NULL);
      if (res < 0)
         dst = NULL;

      enc_len += sz;
   }
   *used = enc_len;

   return res;
}

int XDR_array_decoder(char *src, void *dst, size_t *used, size_t max,
      int len, size_t increment, XDR_Decoder dec, void *dec_arg)
{
   size_t sz = 0, dec_len = 0;
   int i, res;
   char *buff;

   buff = malloc(len * increment);
   if (!buff)
      return -1;
   if (!dst)
      return -2;

   for (i = 0; i < len; i++) {
      sz = 0;
      res = dec(src + dec_len, buff + i*increment, &sz, max - dec_len, NULL);
      if (res < 0)
         return res;
      dec_len += sz;
   }
   *used = dec_len;
   *(char**)dst = buff;

   return 0;
}
