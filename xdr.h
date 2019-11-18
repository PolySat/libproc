#ifndef XDR_H
#define XDR_H

#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "proclib.h"

struct XDR_FieldDefinition;

#ifdef __cplusplus
enum XDR_PRINT_STYLE : short { XDR_PRINT_HUMAN, XDR_PRINT_KVP, XDR_PRINT_CSV_HEADER,
   XDR_PRINT_CSV_DATA };

extern "C" {
#else
enum XDR_PRINT_STYLE { XDR_PRINT_HUMAN, XDR_PRINT_KVP, XDR_PRINT_CSV_HEADER,
   XDR_PRINT_CSV_DATA };
#endif

struct XDR_Hashtable;

typedef int (*XDR_Decoder)(char *src, void *dst, size_t *inc, size_t max,
      void *len);
typedef int (*XDR_Encoder)(char *src, void *dst, size_t *inc, size_t max,
      void *len);
typedef void (*XDR_print_func)(FILE *out, void *data, void *arg, const char *,
      enum XDR_PRINT_STYLE style, int *line, int level);
typedef void (*XDR_print_field_func)(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *len, int *line, int level);
typedef double (*XDR_conversion_func)(double);
typedef void (*XDR_field_scanner)(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
typedef void (*XDR_tx_struct)(void *data, void *arg, uint32_t error);
typedef void (*XDR_populate_struct)(void *arg, XDR_tx_struct cb, void *cb_arg);
typedef int (*XDR_hashtable_itr_cb)(struct XDR_Hashtable *table, const char *key, void *value, void *arg);

#define XDR_HASH_LEN 29

struct XDR_Union {
   uint32_t type;
   void *data;
};

struct XDR_TypeFunctions {
   XDR_Decoder decoder;
   XDR_Encoder encoder;
   XDR_print_field_func printer;
   XDR_field_scanner scanner;
   void (*field_dealloc)(void **, struct XDR_FieldDefinition *field);
};

struct XDR_Hashnode {
   void *data;
   struct XDR_Hashnode *next;
   struct XDR_Hashtable *table;
   char key[2];
};

struct XDR_Hashtable {
   uint32_t length;
   struct XDR_Hashnode *node[XDR_HASH_LEN];
};

struct XDR_FieldDefinition {
   struct XDR_TypeFunctions *funcs;
   size_t offset;
   const char *key;
   const char *name;
   const char *unit;
   XDR_conversion_func conversion;
   XDR_conversion_func inverse_conv;
   uint32_t struct_id;
   const char *description;
   size_t len_offset;
};

struct XDR_StructDefinition {
   uint32_t type;
   size_t in_memory_size;
   int (*encoder)(void *src, char *dst, size_t *inc, size_t max,
         uint32_t type, void *arg);
   int (*decoder)(char *src, void *dst, size_t *used, size_t max, void *arg);
   void *arg;
   void *(*allocator)(struct XDR_StructDefinition *def);
   void (*deallocator)(void **goner, struct XDR_StructDefinition *def);
   XDR_print_func print_func;
   XDR_populate_struct populate;
   void *populate_arg;
};

extern void XDR_register_structs(struct XDR_StructDefinition*);
extern void XDR_register_struct(struct XDR_StructDefinition*);
extern void XDR_register_populator(XDR_populate_struct cb,
      void *arg, uint32_t type);
extern struct XDR_StructDefinition *XDR_definition_for_type(uint32_t type);
extern void XDR_set_struct_print_function(XDR_print_func func, uint32_t type);
extern void XDR_set_field_print_function(XDR_print_field_func func,
      uint32_t struct_type, uint32_t field);

extern int XDR_array_encoder(char *src, void *dst, size_t *used, size_t max,
      int len, size_t increment, XDR_Encoder enc, void *enc_arg);
extern int XDR_array_decoder(char *src, void *dst, size_t *used, size_t max,
      int len, size_t increment, XDR_Decoder dec, void *dec_arg);
extern int XDR_struct_encoder(void *src, char *dst, size_t *encoded_size,
      size_t max, uint32_t type, void *arg);
extern int XDR_bitfield_struct_encoder(void *src, char *dst,
      size_t *encoded_size, size_t max, uint32_t type, void *arg);
extern void XDR_print_structure(uint32_t type,
      struct XDR_StructDefinition *str, char *buff, size_t len, void *arg1,
      int arg2, const char *parent);
extern void XDR_array_field_printer(FILE *out, void *src_ptr,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len_ptr, XDR_print_field_func print, size_t increment,
      const char *parent, int *line, int level);
extern void XDR_array_field_scanner(const char *in, void *dst_ptr, void *arg,
      void *len_ptr, XDR_field_scanner scan, void *enc_arg,
      size_t increment, XDR_conversion_func conv);
extern void XDR_print_fields_func(FILE *out, void *data, void *arg,
      const char*, enum XDR_PRINT_STYLE style, int *line, int level);

extern int XDR_hashtable_encoder(char *src, void *dst, size_t *used, size_t max,
      XDR_Encoder enc, void *enc_arg);
extern int XDR_hashtable_decoder(char *src, struct XDR_Hashtable *dst,
      size_t *used, size_t max, XDR_Decoder dec, size_t ent_size, void *dec_arg);

// All decoder functions take the exact same set of parameters to
//  permit more efficient, generic parsing code
//
// The parameters are:
//  char *src -- The address to start decoding from
//  void *dst -- The address to store the decoded data.  The structure of this 
//                memory will be type dependent
//  size_t *used -- Out parameter that returns the number of bytes used from
//                    the src buffer
//  size_t max -- The total number of bytes available in the src buffer
//
//  Returns negative number on error, or 0 on success.  Positive numbers are
//     reserved for future use
extern int XDR_struct_decoder(char *src, void *dst, size_t *used,
      size_t max, void *arg);
extern int XDR_bitfield_struct_decoder(char *src, void *dst, size_t *used,
      size_t max, void *arg);
extern int XDR_decodebf_int32(char *src, int32_t *dst, size_t *used,
      size_t max, void *len);
extern int XDR_decodebf_uint32(char *src, uint32_t *dst, size_t *used,
      size_t max, void *len);
extern int XDR_decode_int32(char *src, int32_t *dst, size_t *used, size_t max,
      void *len);
extern int XDR_decode_uint32(char *src, uint32_t *dst, size_t *used,
      size_t max, void *len);
extern int XDR_decode_int64(char *src, int64_t *dst, size_t *used, size_t max,
      void *len);
extern int XDR_decode_uint64(char *src, uint64_t *dst, size_t *used,
      size_t max, void *len);
extern int XDR_decode_float(char *src, float *dst, size_t *used, size_t max,
      void *len);
extern int XDR_decode_double(char *src, double *dst, size_t *used, size_t max,
      void *len);
extern int XDR_decode_union(char *src, struct XDR_Union *dst, size_t *used,
      size_t max, void *len);
extern int XDR_decode_string(char *src, char **dst, size_t *used,
      size_t max, void *len);

extern int XDR_decode_byte_array(char *src, char **dst,
      size_t *used, size_t max, void *len);
extern int XDR_decode_uint32_array(char *src, uint32_t **dst,
      size_t *used, size_t max, void *len);
extern int XDR_decode_int32_array(char *src, int32_t **dst,
      size_t *used, size_t max, void *len);
extern int XDR_decode_int64_array(char *src, int64_t **dst, size_t *used,
      size_t max, void *len);
extern int XDR_decode_uint64_array(char *src, uint64_t **dst, size_t *used,
      size_t max, void *len);
extern int XDR_decode_float_array(char *src, float **dst, size_t *used,
      size_t max, void *len);
extern int XDR_decode_double_array(char *src, double **dst, size_t *used,
      size_t max, void *len);
extern int XDR_decode_union_array(char *src, struct XDR_Union **dst,
      size_t *used, size_t max, void *len);
extern int XDR_decode_string_array(char *src, char **dst,
      size_t *used, size_t max, void *len);

extern int XDR_decode_uint32_hashtable(char *src, struct XDR_Hashtable *dst,
      size_t *used, size_t max, void *len);
extern int XDR_decode_int32_hashtable(char *src, struct XDR_Hashtable *dst,
      size_t *used, size_t max, void *len);
extern int XDR_decode_int64_hashtable(char *src, struct XDR_Hashtable *dst,
      size_t *used, size_t max, void *len);
extern int XDR_decode_uint64_hashtable(char *src, struct XDR_Hashtable *dst,
      size_t *used, size_t max, void *len);
extern int XDR_decode_float_hashtable(char *src, struct XDR_Hashtable *dst,
      size_t *used, size_t max, void *len);
extern int XDR_decode_double_hashtable(char *src, struct XDR_Hashtable *dst,
      size_t *used, size_t max, void *len);
extern int XDR_decode_union_hashtable(char *src, struct XDR_Hashtable *dst,
      size_t *used, size_t max, void *len);

extern int XDR_decode_byte_arr_hashtable(char *src, struct XDR_Hashtable *dst,
      size_t *used, size_t max, void *len);
extern int XDR_decode_string_arr_hashtable(char *src, struct XDR_Hashtable *dst,
      size_t *used, size_t max, void *len);

// Encode functions
extern int XDR_encodebf_uint32(uint32_t *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_uint32(uint32_t *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_int32(int32_t *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_int64(int64_t *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_uint64(uint64_t *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_union(struct XDR_Union *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_string(const char *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_float(float *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_double(double *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_byte_array(char **src, char *dst,
      size_t *used, size_t max, void *len);

extern int XDR_encode_uint32_array(uint32_t **src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_int32_array(int32_t **src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_int64_array(int64_t **src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_uint64_array(uint64_t **src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_union_array(struct XDR_Union **src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_float_array(float **src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_double_array(double **src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_string_array(const char **src, char *dst,
      size_t *used, size_t max, void *len);

extern int XDR_encode_uint32_hashtable(uint32_t *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_int32_hashtable(int32_t *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_int64_hashtable(int64_t *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_uint64_hashtable(uint64_t *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_union_hashtable(struct XDR_Union *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_string_hashtable(const char *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_float_hashtable(float *src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_double_hashtable(double *src, char *dst,
      size_t *used, size_t max, void *len);

extern int XDR_encode_byte_arr_hashtable(char **src, char *dst,
      size_t *used, size_t max, void *len);
extern int XDR_encode_string_arr_hashtable(const char **src, char *dst,
      size_t *used, size_t max, void *len);


extern void *XDR_malloc_allocator(struct XDR_StructDefinition*);
extern void XDR_free_deallocator(void **goner, struct XDR_StructDefinition *);
extern void XDR_struct_free_deallocator(void **goner,
      struct XDR_StructDefinition *def);
extern void XDR_struct_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);
extern void XDR_struct_array_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);
extern void XDR_array_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);
extern void XDR_array_hashtable_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);
extern void XDR_union_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);
extern void XDR_union_hashtable_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);
extern void XDR_union_array_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);
extern void XDR_hashtable_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);
extern void XDR_free_union(struct XDR_Union *goner);

extern void XDR_print_field_char(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_int32(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_uint32(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_int64(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_uint64(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_union(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_byte_string(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_string(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_float(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_double(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_structure(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);

extern void XDR_print_field_int32_hashtable(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_uint32_hashtable(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_int64_hashtable(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_uint64_hashtable(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_union_hashtable(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_string_hashtable(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_float_hashtable(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_double_hashtable(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style, 
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_structure_hashtable(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void XDR_print_field_byte_arr_hashtable(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_string_arr_hashtable(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);

extern void XDR_print_field_structure_array(FILE *out, void *src_ptr,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      void *len_ptr, size_t increment, const char *parent, int *line, int level);
extern void XDR_print_field_char_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void XDR_print_field_int32_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void XDR_print_field_uint32_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void XDR_print_field_int64_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void XDR_print_field_uint64_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void XDR_print_field_union_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void XDR_print_field_byte_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *, int *line, int level);
extern void XDR_print_field_float_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void XDR_print_field_double_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void XDR_print_field_string_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);

extern void XDR_scan_float(const char *in, void *dst, void *arg, void *len,
      XDR_conversion_func conv);
extern void XDR_scan_double(const char *in, void *dst, void *arg, void *len,
      XDR_conversion_func conv);
extern void XDR_scan_char(const char *in, void *dst, void *arg, void *len,
      XDR_conversion_func conv);
extern void XDR_scan_int32(const char *in, void *dst, void *arg, void *len,
      XDR_conversion_func conv);
extern void XDR_scan_uint32(const char *in, void *dst, void *arg, void *len,
      XDR_conversion_func conv);
extern void XDR_scan_int64(const char *in, void *dst, void *arg, void *len,
      XDR_conversion_func conv);
extern void XDR_scan_uint64(const char *in, void *dst, void *arg, void *len,
      XDR_conversion_func conv);
extern void XDR_scan_string(const char *in, void *dst, void *arg, void *len,
      XDR_conversion_func conv);
extern void XDR_scan_byte(const char *in, void *dst, void *arg, void *len,
      XDR_conversion_func conv);

extern void XDR_scan_float_hashtable(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_double_hashtable(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_int32_hashtable(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_uint32_hashtable(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_int64_hashtable(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_uint64_hashtable(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_string_arr_hashtable(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_byte_arr_hashtable(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);

extern void XDR_scan_float_array(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_double_array(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_char_array(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_int32_array(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_uint32_array(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_int64_array(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_uint64_array(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_string_array(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);
extern void XDR_scan_byte_array(const char *in, void *dst, void *arg,
      void *len, XDR_conversion_func conv);

extern struct XDR_Hashnode **XDR_hash_lookup_node(struct XDR_Hashtable *table, const char *key);
extern void *XDR_hash_lookup(struct XDR_Hashtable *table, const char *key);
extern void *XDR_hash_remove(struct XDR_Hashtable *table, const char *key);
extern int XDR_hash_remove_all(struct XDR_Hashtable *table, XDR_hashtable_itr_cb freeCB, void *arg);
extern int XDR_hash_add(struct XDR_Hashtable *table, const char *key, void *value);
extern void XDR_hash_iterate(struct XDR_Hashtable *table, XDR_hashtable_itr_cb cb, void *arg);
extern char *XDR_hash_lookup_value(struct XDR_Hashtable *table, void *val);
extern int XDR_hash_bucket ( const char * key);

extern struct XDR_TypeFunctions xdr_float_functions;
extern struct XDR_TypeFunctions xdr_float_arr_functions;
extern struct XDR_TypeFunctions xdr_float_ht_functions;
extern struct XDR_TypeFunctions xdr_double_functions;
extern struct XDR_TypeFunctions xdr_double_arr_functions;
extern struct XDR_TypeFunctions xdr_double_ht_functions;
extern struct XDR_TypeFunctions xdr_char_functions;
extern struct XDR_TypeFunctions xdr_char_arr_functions;
extern struct XDR_TypeFunctions xdr_int32_functions;
extern struct XDR_TypeFunctions xdr_int32_arr_functions;
extern struct XDR_TypeFunctions xdr_int32_ht_functions;
extern struct XDR_TypeFunctions xdr_uint32_functions;
extern struct XDR_TypeFunctions xdr_uint32_arr_functions;
extern struct XDR_TypeFunctions xdr_uint32_ht_functions;
extern struct XDR_TypeFunctions xdr_int64_functions;
extern struct XDR_TypeFunctions xdr_int64_arr_functions;
extern struct XDR_TypeFunctions xdr_int64_ht_functions;
extern struct XDR_TypeFunctions xdr_uint64_functions;
extern struct XDR_TypeFunctions xdr_uint64_arr_functions;
extern struct XDR_TypeFunctions xdr_uint64_ht_functions;
extern struct XDR_TypeFunctions xdr_string_functions;
extern struct XDR_TypeFunctions xdr_string_arr_functions;
extern struct XDR_TypeFunctions xdr_string_arr_ht_functions;
extern struct XDR_TypeFunctions xdr_byte_arr_functions;
extern struct XDR_TypeFunctions xdr_byte_arr_ht_functions;
extern struct XDR_TypeFunctions xdr_union_functions;
extern struct XDR_TypeFunctions xdr_union_arr_functions;
extern struct XDR_TypeFunctions xdr_union_ht_functions;
extern struct XDR_TypeFunctions xdr_int32_bitfield_functions;
extern struct XDR_TypeFunctions xdr_uint32_bitfield_functions;

#ifdef __cplusplus
}
#endif

#endif
