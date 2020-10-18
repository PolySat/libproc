#include <stddef.h>
#include <polysat/cmd.h>
#include <polysat/cmd-pkt.h>
#include "test_schema.h"

void IPC_TEST_forcelink(void) {}

struct XDR_TypeFunctions IPC_TEST_DictTest_functions = {
   (XDR_Decoder)&IPC_TEST_DictTest_decode,
   (XDR_Encoder)&IPC_TEST_DictTest_encode,
   &XDR_print_field_structure, NULL,
   &IPC_TEST_DictTest_field_deallocator
};

struct XDR_TypeFunctions IPC_TEST_DictTest_arr_functions = {
   (XDR_Decoder)&IPC_TEST_DictTest_decode_array,
   (XDR_Encoder)&IPC_TEST_DictTest_encode_array,
   &IPC_TEST_DictTest_print_array, NULL,
   &XDR_struct_array_field_deallocator
};

struct XDR_TypeFunctions IPC_TEST_DictTest_dict_functions = {
   (XDR_Decoder)&IPC_TEST_DictTest_decode_dictionary,
   (XDR_Encoder)&IPC_TEST_DictTest_encode_dictionary,
   &IPC_TEST_DictTest_print_dictionary, NULL,
   &IPC_TEST_DictTest_dictionary_field_deallocator
};

struct XDR_TypeFunctions IPC_TEST_Status_functions = {
   (XDR_Decoder)&IPC_TEST_Status_decode,
   (XDR_Encoder)&IPC_TEST_Status_encode,
   &XDR_print_field_structure, NULL,
   &IPC_TEST_Status_field_deallocator
};

struct XDR_TypeFunctions IPC_TEST_Status_arr_functions = {
   (XDR_Decoder)&IPC_TEST_Status_decode_array,
   (XDR_Encoder)&IPC_TEST_Status_encode_array,
   &IPC_TEST_Status_print_array, NULL,
   &XDR_struct_array_field_deallocator
};

struct XDR_TypeFunctions IPC_TEST_Status_dict_functions = {
   (XDR_Decoder)&IPC_TEST_Status_decode_dictionary,
   (XDR_Encoder)&IPC_TEST_Status_encode_dictionary,
   &IPC_TEST_Status_print_dictionary, NULL,
   &IPC_TEST_Status_dictionary_field_deallocator
};

struct XDR_TypeFunctions IPC_TEST_PTest_functions = {
   (XDR_Decoder)&IPC_TEST_PTest_decode,
   (XDR_Encoder)&IPC_TEST_PTest_encode,
   &XDR_print_field_structure, NULL,
   &IPC_TEST_PTest_field_deallocator
};

struct XDR_TypeFunctions IPC_TEST_PTest_arr_functions = {
   (XDR_Decoder)&IPC_TEST_PTest_decode_array,
   (XDR_Encoder)&IPC_TEST_PTest_encode_array,
   &IPC_TEST_PTest_print_array, NULL,
   &XDR_struct_array_field_deallocator
};

struct XDR_TypeFunctions IPC_TEST_PTest_dict_functions = {
   (XDR_Decoder)&IPC_TEST_PTest_decode_dictionary,
   (XDR_Encoder)&IPC_TEST_PTest_encode_dictionary,
   &IPC_TEST_PTest_print_dictionary, NULL,
   &IPC_TEST_PTest_dictionary_field_deallocator
};

static struct XDR_FieldDefinition IPC_TEST_DictTest_Fields[] = {
   { &xdr_int32_functions,
      offsetof(struct IPC_TEST_DictTest, field1),
      "int1", "Int1", NULL,
      NULL,
      NULL,
      0,
      NULL,
      0 },

   { &xdr_string_arr_functions,
      offsetof(struct IPC_TEST_DictTest, str),
      "test_str", "Test String", NULL,
      NULL,
      NULL,
      0,
      NULL,
      0 },

   { &xdr_int32_functions,
      offsetof(struct IPC_TEST_DictTest, field2),
      "int2", "Int2", NULL,
      NULL,
      NULL,
      0,
      NULL,
      0 },

   { NULL, 0, NULL, NULL, NULL, NULL, NULL, 0, NULL, 0 }
};

static struct XDR_StructDefinition IPC_TEST_DictTest_Struct = {
   IPC_TEST_DATA_TYPES_DICT_STR, sizeof(struct IPC_TEST_DictTest),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_TEST_DictTest_Fields,
   &XDR_malloc_allocator, &XDR_struct_free_deallocator, &XDR_print_fields_func,
   NULL, NULL
};

static struct XDR_FieldDefinition IPC_TEST_Status_Fields[] = {
   { &xdr_int32_functions,
      offsetof(struct IPC_TEST_Status, foo),
      "bar", "John", "units",
      NULL,
      NULL,
      0,
      "This is a longer test",
      0 },

   { &xdr_uint64_functions,
      offsetof(struct IPC_TEST_Status, bar),
      NULL, "Test 2", NULL,
      NULL,
      NULL,
      0,
      NULL,
      0 },

   { &IPC_TEST_DictTest_dict_functions,
      offsetof(struct IPC_TEST_Status, values),
      "test_dict", "test dict", NULL,
      NULL,
      NULL,
      IPC_TEST_DATA_TYPES_DICT_STR,
      NULL,
      0 },

   { &xdr_int32_dict_functions,
      offsetof(struct IPC_TEST_Status, intdict),
      "int_dict", "test int dict", NULL,
      NULL,
      NULL,
      0,
      NULL,
      0 },

   { &IPC_TEST_DictTest_functions,
      offsetof(struct IPC_TEST_Status, test),
      "str_test", "test structure", NULL,
      NULL,
      NULL,
      IPC_TEST_DATA_TYPES_DICT_STR,
      NULL,
      0 },

   { NULL, 0, NULL, NULL, NULL, NULL, NULL, 0, NULL, 0 }
};

static struct XDR_StructDefinition IPC_TEST_Status_Struct = {
   IPC_TEST_DATA_TYPES_STATUS, sizeof(struct IPC_TEST_Status),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_TEST_Status_Fields,
   &XDR_malloc_allocator, &XDR_struct_free_deallocator, &XDR_print_fields_func,
   NULL, NULL
};

static struct XDR_FieldDefinition IPC_TEST_PTest_Fields[] = {
   { &xdr_int32_functions,
      offsetof(struct IPC_TEST_PTest, val1),
      "param1", NULL, NULL,
      NULL,
      NULL,
      0,
      "The first test value",
      0 },

   { &xdr_int32_functions,
      offsetof(struct IPC_TEST_PTest, val2),
      "val2", NULL, NULL,
      NULL,
      NULL,
      0,
      "the second value",
      0 },

   { &xdr_int32_functions,
      offsetof(struct IPC_TEST_PTest, val3),
      "polysat", NULL, NULL,
      NULL,
      NULL,
      0,
      "Cal Poly Cubesat Lab",
      0 },

   { &xdr_int32_functions,
      offsetof(struct IPC_TEST_PTest, val4),
      "test", NULL, NULL,
      NULL,
      NULL,
      0,
      "a test value",
      0 },

   { NULL, 0, NULL, NULL, NULL, NULL, NULL, 0, NULL, 0 }
};

static struct XDR_StructDefinition IPC_TEST_PTest_Struct = {
   IPC_TEST_DATA_TYPES_PTEST, sizeof(struct IPC_TEST_PTest),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_TEST_PTest_Fields,
   &XDR_malloc_allocator, &XDR_struct_free_deallocator, &XDR_print_fields_func,
   NULL, NULL
};

int IPC_TEST_DictTest_decode(char *src,
      struct IPC_TEST_DictTest *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_TEST_DictTest_Fields);
}

int IPC_TEST_DictTest_encode(
      struct IPC_TEST_DictTest *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TEST_DATA_TYPES_DICT_STR , IPC_TEST_DictTest_Fields);
}

int IPC_TEST_DictTest_decode_array(char *src,
      struct IPC_TEST_DictTest **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_TEST_DictTest),
            (XDR_Decoder)&IPC_TEST_DictTest_decode, NULL);

   return 0;
}

int IPC_TEST_DictTest_encode_array(
      struct IPC_TEST_DictTest **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_TEST_DictTest),
            (XDR_Encoder)&IPC_TEST_DictTest_encode, NULL);

   return 0;
}

int IPC_TEST_DictTest_decode_dictionary(char *src,
      struct XDR_Dictionary *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_dictionary_decoder(src, dst, used, max,
         (XDR_Decoder)&IPC_TEST_DictTest_decode,
         sizeof(struct IPC_TEST_DictTest), NULL);
}

int IPC_TEST_DictTest_encode_dictionary(
      struct XDR_Dictionary *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_dictionary_encoder(src, dst, used, max,
         (XDR_Encoder)&IPC_TEST_DictTest_encode,
         sizeof(struct IPC_TEST_DictTest), NULL);
}

void IPC_TEST_DictTest_print_dictionary(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level)
{
   XDR_dictionary_field_printer(out, data, field, style,
         len, &XDR_print_field_structure, 0, parent, line, level);
}
int IPC_TEST_Status_decode(char *src,
      struct IPC_TEST_Status *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_TEST_Status_Fields);
}

int IPC_TEST_Status_encode(
      struct IPC_TEST_Status *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TEST_DATA_TYPES_STATUS , IPC_TEST_Status_Fields);
}

int IPC_TEST_Status_decode_array(char *src,
      struct IPC_TEST_Status **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_TEST_Status),
            (XDR_Decoder)&IPC_TEST_Status_decode, NULL);

   return 0;
}

int IPC_TEST_Status_encode_array(
      struct IPC_TEST_Status **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_TEST_Status),
            (XDR_Encoder)&IPC_TEST_Status_encode, NULL);

   return 0;
}

int IPC_TEST_Status_decode_dictionary(char *src,
      struct XDR_Dictionary *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_dictionary_decoder(src, dst, used, max,
         (XDR_Decoder)&IPC_TEST_Status_decode,
         sizeof(struct IPC_TEST_Status), NULL);
}

int IPC_TEST_Status_encode_dictionary(
      struct XDR_Dictionary *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_dictionary_encoder(src, dst, used, max,
         (XDR_Encoder)&IPC_TEST_Status_encode,
         sizeof(struct IPC_TEST_Status), NULL);
}

void IPC_TEST_Status_print_dictionary(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level)
{
   XDR_dictionary_field_printer(out, data, field, style,
         len, &XDR_print_field_structure, 0, parent, line, level);
}
int IPC_TEST_PTest_decode(char *src,
      struct IPC_TEST_PTest *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_TEST_PTest_Fields);
}

int IPC_TEST_PTest_encode(
      struct IPC_TEST_PTest *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TEST_DATA_TYPES_PTEST , IPC_TEST_PTest_Fields);
}

int IPC_TEST_PTest_decode_array(char *src,
      struct IPC_TEST_PTest **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_TEST_PTest),
            (XDR_Decoder)&IPC_TEST_PTest_decode, NULL);

   return 0;
}

int IPC_TEST_PTest_encode_array(
      struct IPC_TEST_PTest **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_TEST_PTest),
            (XDR_Encoder)&IPC_TEST_PTest_encode, NULL);

   return 0;
}

int IPC_TEST_PTest_decode_dictionary(char *src,
      struct XDR_Dictionary *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_dictionary_decoder(src, dst, used, max,
         (XDR_Decoder)&IPC_TEST_PTest_decode,
         sizeof(struct IPC_TEST_PTest), NULL);
}

int IPC_TEST_PTest_encode_dictionary(
      struct XDR_Dictionary *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_dictionary_encoder(src, dst, used, max,
         (XDR_Encoder)&IPC_TEST_PTest_encode,
         sizeof(struct IPC_TEST_PTest), NULL);
}

void IPC_TEST_PTest_print_dictionary(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level)
{
   XDR_dictionary_field_printer(out, data, field, style,
         len, &XDR_print_field_structure, 0, parent, line, level);
}
void IPC_TEST_DictTest_print_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level)
{
   XDR_print_field_structure_array(out, data, field, style, len,
         sizeof(struct IPC_TEST_DictTest), parent, line, level);
}

void IPC_TEST_DictTest_dictionary_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field)
{
   struct XDR_Dictionary *table = (struct XDR_Dictionary*)goner;

   if (!goner || !field)
      return;
   XDR_dict_remove_all(table, &XDR_dictionary_free_cb,
         &IPC_TEST_DictTest_Struct);
}

void IPC_TEST_DictTest_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field)
{
   XDR_struct_free_fields((void**)&goner,
         &IPC_TEST_DictTest_Struct);
}

void IPC_TEST_Status_print_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level)
{
   XDR_print_field_structure_array(out, data, field, style, len,
         sizeof(struct IPC_TEST_Status), parent, line, level);
}

void IPC_TEST_Status_dictionary_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field)
{
   struct XDR_Dictionary *table = (struct XDR_Dictionary*)goner;

   if (!goner || !field)
      return;
   XDR_dict_remove_all(table, &XDR_dictionary_free_cb,
         &IPC_TEST_Status_Struct);
}

void IPC_TEST_Status_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field)
{
   XDR_struct_free_fields((void**)&goner,
         &IPC_TEST_Status_Struct);
}

void IPC_TEST_PTest_print_array(FILE *out, void *data,
      struct XDR_FieldDefinition *field, enum XDR_PRINT_STYLE style,
      const char *parent, void *len, int *line, int level)
{
   XDR_print_field_structure_array(out, data, field, style, len,
         sizeof(struct IPC_TEST_PTest), parent, line, level);
}

void IPC_TEST_PTest_dictionary_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field)
{
   struct XDR_Dictionary *table = (struct XDR_Dictionary*)goner;

   if (!goner || !field)
      return;
   XDR_dict_remove_all(table, &XDR_dictionary_free_cb,
         &IPC_TEST_PTest_Struct);
}

void IPC_TEST_PTest_field_deallocator(void **goner,
      struct XDR_FieldDefinition *field)
{
   XDR_struct_free_fields((void**)&goner,
         &IPC_TEST_PTest_Struct);
}

static struct CMD_XDRCommandInfo IPC_test_Commands[] = {
   { IPC_CMDS_STATUS, 0,
     "test-status",
     "Returns the process' status",
     NULL,
     NULL, NULL, NULL },
   { IPC_TEST_COMMANDS_PTEST, IPC_TEST_DATA_TYPES_PTEST,
     "test-param",
     "A command to use when testing command parameters",
     NULL,
     NULL, NULL, NULL },
   { 0, 0, NULL, NULL, NULL, NULL, NULL, NULL }
};

static struct CMD_ErrorInfo IPC_test_Errors[] = {
   { 0, NULL, NULL }
};

static void IPC_TEST_structs_constructor()
    __attribute__((constructor));

static void IPC_TEST_structs_constructor()
{
   XDR_register_struct(&IPC_TEST_DictTest_Struct);
   XDR_register_struct(&IPC_TEST_Status_Struct);
   XDR_register_struct(&IPC_TEST_PTest_Struct);
   CMD_register_commands(IPC_test_Commands, 1);
   CMD_register_errors(IPC_test_Errors);
}
