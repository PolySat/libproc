#ifndef TEST_SCHEMA_H
#define TEST_SCHEMA_H

#include "xdr.h"
#include "cmd.h"
#include <stdint.h>

#define IPC_TEST_BASE (512)

enum IPC_TEST_DATA_TYPES {
   IPC_TEST_DATA_TYPES_STATUS = IPC_TEST_BASE + 1,
   IPC_TEST_DATA_TYPES_PTEST = IPC_TEST_BASE + 2,
};

enum IPC_TEST_COMMANDS {
   IPC_TEST_COMMANDS_STATUS = IPC_TEST_BASE + 1,
   IPC_TEST_COMMANDS_PTEST = IPC_TEST_BASE + 2,
};

struct IPC_TEST_Status {
   int32_t foo;
   uint64_t bar;
};

struct IPC_TEST_PTest {
   int32_t val1;
   int32_t val2;
   int32_t val3;
   int32_t val4;
};

extern int IPC_TEST_Status_decode(char *src,
      struct IPC_TEST_Status *dst, size_t *used,
      size_t max);
extern int IPC_TEST_Status_encode(
      struct IPC_TEST_Status *src, char *dst, size_t *used,
      size_t max);

extern int IPC_TEST_PTest_decode(char *src,
      struct IPC_TEST_PTest *dst, size_t *used,
      size_t max);
extern int IPC_TEST_PTest_encode(
      struct IPC_TEST_PTest *src, char *dst, size_t *used,
      size_t max);

#endif
