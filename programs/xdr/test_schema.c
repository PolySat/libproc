#include <stddef.h>
#include "cmd.h"
#include "cmd_schema.h"
#include "test_schema.h"

static struct XDR_FieldDefinition IPC_TEST_Status_Fields[] = {
   { (XDR_Decoder)&XDR_decode_int32,
      (XDR_Encoder)&XDR_encode_int32,
      offsetof(struct IPC_TEST_Status, foo),
      "bar", "John", "units", 0, 1,
      &XDR_print_field_int32, &XDR_scan_int32,
      "This is a longer test" },

   { (XDR_Decoder)&XDR_decode_uint64,
      (XDR_Encoder)&XDR_encode_uint64,
      offsetof(struct IPC_TEST_Status, bar),
      NULL, "Test 2", NULL, 0, 0,
      &XDR_print_field_uint64, &XDR_scan_uint64,
      NULL },

   { NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL }
};

static struct XDR_StructDefinition IPC_TEST_Status_Struct = {
   IPC_TEST_DATA_TYPES_STATUS, sizeof(struct IPC_TEST_Status),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_TEST_Status_Fields,
   &XDR_malloc_allocator, &XDR_free_deallocator, &XDR_print_fields_func
};

static struct XDR_FieldDefinition IPC_TEST_PTest_Fields[] = {
   { (XDR_Decoder)&XDR_decode_int32,
      (XDR_Encoder)&XDR_encode_int32,
      offsetof(struct IPC_TEST_PTest, val1),
      "param1", NULL, NULL, 0, 0,
      &XDR_print_field_int32, &XDR_scan_int32,
      "The first test value" },

   { (XDR_Decoder)&XDR_decode_int32,
      (XDR_Encoder)&XDR_encode_int32,
      offsetof(struct IPC_TEST_PTest, val2),
      "val2", NULL, NULL, 0, 0,
      &XDR_print_field_int32, &XDR_scan_int32,
      NULL },

   { (XDR_Decoder)&XDR_decode_int32,
      (XDR_Encoder)&XDR_encode_int32,
      offsetof(struct IPC_TEST_PTest, val3),
      "polysat", NULL, NULL, 0, 0,
      &XDR_print_field_int32, &XDR_scan_int32,
      NULL },

   { (XDR_Decoder)&XDR_decode_int32,
      (XDR_Encoder)&XDR_encode_int32,
      offsetof(struct IPC_TEST_PTest, val4),
      "test", NULL, NULL, 0, 0,
      &XDR_print_field_int32, &XDR_scan_int32,
      NULL },

   { NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL }
};

static struct XDR_StructDefinition IPC_TEST_PTest_Struct = {
   IPC_TEST_DATA_TYPES_PTEST, sizeof(struct IPC_TEST_PTest),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_TEST_PTest_Fields,
   &XDR_malloc_allocator, &XDR_free_deallocator, &XDR_print_fields_func
};

int IPC_TEST_Status_decode(char *src,
      struct IPC_TEST_Status *dst, size_t *used,
      size_t max)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_TEST_Status_Fields);
}

int IPC_TEST_Status_encode(
      struct IPC_TEST_Status *src, char *dst, size_t *used,
      size_t max)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TEST_DATA_TYPES_STATUS , IPC_TEST_Status_Fields);
}

int IPC_TEST_PTest_decode(char *src,
      struct IPC_TEST_PTest *dst, size_t *used,
      size_t max)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_TEST_PTest_Fields);
}

int IPC_TEST_PTest_encode(
      struct IPC_TEST_PTest *src, char *dst, size_t *used,
      size_t max)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TEST_DATA_TYPES_PTEST , IPC_TEST_PTest_Fields);
}

static struct CMD_XDRCommandInfo IPC_test_Commands[] = {
   { IPC_COMMANDS_STATUS, 0,
     "test-status",
     "Returns the process' status",
     NULL, NULL, NULL },
   { IPC_TEST_COMMANDS_PTEST, IPC_TEST_DATA_TYPES_PTEST,
     "test-param",
     "A command to use when testing command parameters",
     NULL, NULL, NULL },
   { 0, 0, NULL, NULL, NULL, NULL, NULL }
};

static struct CMD_ErrorInfo IPC_test_Errors[] = {
   { 0, NULL, NULL }
};

static void IPC_TEST_structs_constructor()
    __attribute__((constructor));

static void IPC_TEST_structs_constructor()
{
   XDR_register_struct(&IPC_TEST_Status_Struct);
   XDR_register_struct(&IPC_TEST_PTest_Struct);
   CMD_register_commands(IPC_test_Commands, 1);
   CMD_register_errors(IPC_test_Errors);
}
