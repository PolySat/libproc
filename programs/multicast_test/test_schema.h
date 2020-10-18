#ifndef TEST_SCHEMA_H
#define TEST_SCHEMA_H

#include <polysat/xdr.h>
#include <polysat/cmd.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPC_TEST_BASE (512)

enum IPC_TEST_DATA_TYPES {
   IPC_TEST_DATA_TYPES_STATUS = IPC_TEST_BASE + 1,
   IPC_TEST_DATA_TYPES_PTEST = IPC_TEST_BASE + 2,
   IPC_TEST_DATA_TYPES_DICT_STR = IPC_TEST_BASE + 3,
};

enum IPC_TEST_COMMANDS {
   IPC_TEST_COMMANDS_STATUS = IPC_TEST_BASE + 1,
   IPC_TEST_COMMANDS_PTEST = IPC_TEST_BASE + 2,
};

enum IPC_TEST_DICTTEST_FIELDS {
   IPC_TEST_DICTTEST_FIELDS_FIELD1 = 0,
   IPC_TEST_DICTTEST_FIELDS_STR = 1,
   IPC_TEST_DICTTEST_FIELDS_FIELD2 = 2,
};

struct IPC_TEST_DictTest {
   int32_t field1;
   const char *str;
   int32_t field2;
};

enum IPC_TEST_STATUS_FIELDS {
   IPC_TEST_STATUS_FIELDS_FOO = 0,
   IPC_TEST_STATUS_FIELDS_BAR = 1,
   IPC_TEST_STATUS_FIELDS_VALUES = 2,
   IPC_TEST_STATUS_FIELDS_INTDICT = 3,
   IPC_TEST_STATUS_FIELDS_TEST = 4,
};

struct IPC_TEST_Status {
   int32_t foo;
   uint64_t bar;
   struct XDR_Dictionary  values;
   struct XDR_Dictionary intdict;
   struct IPC_TEST_DictTest test;
};

enum IPC_TEST_PTEST_FIELDS {
   IPC_TEST_PTEST_FIELDS_VAL1 = 0,
   IPC_TEST_PTEST_FIELDS_VAL2 = 1,
   IPC_TEST_PTEST_FIELDS_VAL3 = 2,
   IPC_TEST_PTEST_FIELDS_VAL4 = 3,
};

struct IPC_TEST_PTest {
   int32_t val1;
   int32_t val2;
   int32_t val3;
   int32_t val4;
};

extern int IPC_TEST_DictTest_decode(char *src,
      struct IPC_TEST_DictTest *dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_DictTest_encode(
      struct IPC_TEST_DictTest *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_DictTest_decode_array(char *src,
      struct IPC_TEST_DictTest **dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_DictTest_encode_array(
      struct IPC_TEST_DictTest **src, char *dst, size_t *used,
      size_t max, void *len);
extern void IPC_TEST_DictTest_print_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern int IPC_TEST_DictTest_decode_dictionary(char *src,
      struct XDR_Dictionary *dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_DictTest_encode_dictionary(
      struct XDR_Dictionary *src, char *dst, size_t *used,
      size_t max, void *len);
extern void IPC_TEST_DictTest_print_dictionary(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void IPC_TEST_DictTest_dictionary_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);
extern void IPC_TEST_DictTest_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);

extern int IPC_TEST_Status_decode(char *src,
      struct IPC_TEST_Status *dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_Status_encode(
      struct IPC_TEST_Status *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_Status_decode_array(char *src,
      struct IPC_TEST_Status **dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_Status_encode_array(
      struct IPC_TEST_Status **src, char *dst, size_t *used,
      size_t max, void *len);
extern void IPC_TEST_Status_print_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern int IPC_TEST_Status_decode_dictionary(char *src,
      struct XDR_Dictionary *dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_Status_encode_dictionary(
      struct XDR_Dictionary *src, char *dst, size_t *used,
      size_t max, void *len);
extern void IPC_TEST_Status_print_dictionary(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void IPC_TEST_Status_dictionary_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);
extern void IPC_TEST_Status_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);

extern int IPC_TEST_PTest_decode(char *src,
      struct IPC_TEST_PTest *dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_PTest_encode(
      struct IPC_TEST_PTest *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_PTest_decode_array(char *src,
      struct IPC_TEST_PTest **dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_PTest_encode_array(
      struct IPC_TEST_PTest **src, char *dst, size_t *used,
      size_t max, void *len);
extern void IPC_TEST_PTest_print_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern int IPC_TEST_PTest_decode_dictionary(char *src,
      struct XDR_Dictionary *dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_PTest_encode_dictionary(
      struct XDR_Dictionary *src, char *dst, size_t *used,
      size_t max, void *len);
extern void IPC_TEST_PTest_print_dictionary(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);
extern void IPC_TEST_PTest_dictionary_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);
extern void IPC_TEST_PTest_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field);

extern void IPC_TEST_forcelink(void);

#ifdef __cplusplus
}
#endif

#endif
