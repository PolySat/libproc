#ifndef TEST_SCHEMA_H
#define TEST_SCHEMA_H

#include <polysat3/xdr.h>
#include <polysat3/cmd.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPC_TEST_BASE (512)

enum IPC_TEST_DATA_TYPES {
   IPC_TEST_DATA_TYPES_STATUS = IPC_TEST_BASE + 1,
   IPC_TEST_DATA_TYPES_PTEST = IPC_TEST_BASE + 2,
   IPC_TEST_DATA_TYPES_MCAST = IPC_TEST_BASE + 3,
};

enum IPC_TEST_COMMANDS {
   IPC_TEST_COMMANDS_STATUS = IPC_TEST_BASE + 1,
   IPC_TEST_COMMANDS_PTEST = IPC_TEST_BASE + 2,
   IPC_TEST_COMMANDS_MCAST_TEST = IPC_TEST_BASE + 3,
};

enum IPC_TEST_STATUS_FIELDS {
   IPC_TEST_STATUS_FIELDS_FOO = 0,
   IPC_TEST_STATUS_FIELDS_BAR = 1,
};

struct IPC_TEST_Status {
   int32_t foo;
   uint64_t bar;
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

enum IPC_TEST_MCAST_FIELDS {
   IPC_TEST_MCAST_FIELDS_VAL1 = 0,
   IPC_TEST_MCAST_FIELDS_VAL2 = 1,
};

struct IPC_TEST_Mcast {
   int32_t val1;
   int32_t val2;
};

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

extern int IPC_TEST_Mcast_decode(char *src,
      struct IPC_TEST_Mcast *dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_Mcast_encode(
      struct IPC_TEST_Mcast *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_Mcast_decode_array(char *src,
      struct IPC_TEST_Mcast **dst, size_t *used,
      size_t max, void *len);
extern int IPC_TEST_Mcast_encode_array(
      struct IPC_TEST_Mcast **src, char *dst, size_t *used,
      size_t max, void *len);
extern void IPC_TEST_Mcast_print_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level);

extern void IPC_TEST_forcelink(void);

#ifdef __cplusplus
}
#endif

#endif
