#include <stddef.h>
#include "cmd.h"
#include "cmd-pkt.h"

void IPC_forcelink(void) {}

static struct XDR_FieldDefinition IPC_Void_Fields[] = {
   { NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL, 0 }
};

static struct XDR_StructDefinition IPC_Void_Struct = {
   IPC_TYPES_VOID, sizeof(struct IPC_Void),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_Void_Fields,
   &XDR_malloc_allocator, &XDR_struct_free_deallocator, &XDR_print_fields_func,
   NULL, NULL
};

static struct XDR_FieldDefinition IPC_Command_Fields[] = {
   { (XDR_Decoder)&XDR_decode_uint32,
      (XDR_Encoder)&XDR_encode_uint32,
      offsetof(struct IPC_Command, cmd),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_uint32, &XDR_scan_uint32,
      NULL, 0,
      NULL,
      0 },

   { (XDR_Decoder)&XDR_decode_uint32,
      (XDR_Encoder)&XDR_encode_uint32,
      offsetof(struct IPC_Command, ipcref),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_uint32, &XDR_scan_uint32,
      NULL, 0,
      NULL,
      0 },

   { (XDR_Decoder)&XDR_decode_union,
      (XDR_Encoder)&XDR_encode_union,
      offsetof(struct IPC_Command, parameters),
      NULL, NULL, NULL, 0, 0,
      NULL, NULL,
      &XDR_union_field_deallocator, 0,
      NULL,
      0 },

   { NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL, 0 }
};

static struct XDR_StructDefinition IPC_Command_Struct = {
   IPC_TYPES_COMMAND, sizeof(struct IPC_Command),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_Command_Fields,
   &XDR_malloc_allocator, &XDR_struct_free_deallocator, &XDR_print_fields_func,
   NULL, NULL
};

static struct XDR_FieldDefinition IPC_Heartbeat_Fields[] = {
   { (XDR_Decoder)&XDR_decode_uint64,
      (XDR_Encoder)&XDR_encode_uint64,
      offsetof(struct IPC_Heartbeat, commands),
      "proc_commands", "Commands", NULL, 0, 0,
      &XDR_print_field_uint64, &XDR_scan_uint64,
      NULL, 0,
      "The number of commands received by the process",
      0 },

   { (XDR_Decoder)&XDR_decode_uint64,
      (XDR_Encoder)&XDR_encode_uint64,
      offsetof(struct IPC_Heartbeat, responses),
      "proc_responses", "Responses", NULL, 0, 0,
      &XDR_print_field_uint64, &XDR_scan_uint64,
      NULL, 0,
      "The number of command responses received by the process",
      0 },

   { (XDR_Decoder)&XDR_decode_uint64,
      (XDR_Encoder)&XDR_encode_uint64,
      offsetof(struct IPC_Heartbeat, heartbeats),
      "proc_heartbeats", "Heartbeats", NULL, 0, 0,
      &XDR_print_field_uint64, &XDR_scan_uint64,
      NULL, 0,
      "The number of heartbeat commands received by the process",
      0 },

   { NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL, 0 }
};

static struct XDR_StructDefinition IPC_Heartbeat_Struct = {
   IPC_TYPES_HEARTBEAT, sizeof(struct IPC_Heartbeat),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_Heartbeat_Fields,
   &XDR_malloc_allocator, &XDR_struct_free_deallocator, &XDR_print_fields_func,
   NULL, NULL
};

static struct XDR_FieldDefinition IPC_DataReq_Fields[] = {
   { (XDR_Decoder)&XDR_decode_int32,
      (XDR_Encoder)&XDR_encode_int32,
      offsetof(struct IPC_DataReq, length),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_int32, &XDR_scan_int32,
      NULL, 0,
      NULL,
      0 },

   { (XDR_Decoder)&XDR_decode_uint32_array,
      (XDR_Encoder)&XDR_encode_uint32_array,
      offsetof(struct IPC_DataReq, reqs),
      "types", NULL, NULL, 0, 0,
      &XDR_print_field_uint32_array, &XDR_scan_uint32_array,
      &XDR_array_field_deallocator, 0,
      NULL,
      offsetof(struct IPC_DataReq, length) },

   { NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL, 0 }
};

static struct XDR_StructDefinition IPC_DataReq_Struct = {
   IPC_TYPES_DATAREQ, sizeof(struct IPC_DataReq),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_DataReq_Fields,
   &XDR_malloc_allocator, &XDR_struct_free_deallocator, &XDR_print_fields_func,
   NULL, NULL
};

static struct XDR_FieldDefinition IPC_OpaqueStruct_Fields[] = {
   { (XDR_Decoder)&XDR_decode_int32,
      (XDR_Encoder)&XDR_encode_int32,
      offsetof(struct IPC_OpaqueStruct, length),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_int32, &XDR_scan_int32,
      NULL, 0,
      NULL,
      0 },

   { (XDR_Decoder)&XDR_decode_byte_array,
      (XDR_Encoder)&XDR_encode_byte_array,
      offsetof(struct IPC_OpaqueStruct, data),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_byte_array, &XDR_scan_byte_array,
      &XDR_array_field_deallocator, 0,
      NULL,
      offsetof(struct IPC_OpaqueStruct, length) },

   { NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL, 0 }
};

static struct XDR_StructDefinition IPC_OpaqueStruct_Struct = {
   IPC_TYPES_OPAQUE_STRUCT, sizeof(struct IPC_OpaqueStruct),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_OpaqueStruct_Fields,
   &XDR_malloc_allocator, &XDR_struct_free_deallocator, &XDR_print_fields_func,
   NULL, NULL
};

static struct XDR_FieldDefinition IPC_OpaqueStructArr_Fields[] = {
   { (XDR_Decoder)&XDR_decode_int32,
      (XDR_Encoder)&XDR_encode_int32,
      offsetof(struct IPC_OpaqueStructArr, length),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_int32, &XDR_scan_int32,
      NULL, 0,
      NULL,
      0 },

   { (XDR_Decoder)&IPC_OpaqueStruct_decode_array,
      (XDR_Encoder)&IPC_OpaqueStruct_encode_array,
      offsetof(struct IPC_OpaqueStructArr, structs),
      NULL, NULL, NULL, 0, 0,
      NULL, NULL,
      &XDR_struct_array_field_deallocator, IPC_TYPES_OPAQUE_STRUCT,
      NULL,
      offsetof(struct IPC_OpaqueStructArr, length) },

   { NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL, 0 }
};

static struct XDR_StructDefinition IPC_OpaqueStructArr_Struct = {
   IPC_TYPES_OPAQUE_STRUCT_ARR, sizeof(struct IPC_OpaqueStructArr),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_OpaqueStructArr_Fields,
   &XDR_malloc_allocator, &XDR_struct_free_deallocator, &XDR_print_fields_func,
   NULL, NULL
};

static struct XDR_FieldDefinition IPC_Response_Fields[] = {
   { (XDR_Decoder)&XDR_decode_uint32,
      (XDR_Encoder)&XDR_encode_uint32,
      offsetof(struct IPC_Response, cmd),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_uint32, &XDR_scan_uint32,
      NULL, 0,
      NULL,
      0 },

   { (XDR_Decoder)&XDR_decode_uint32,
      (XDR_Encoder)&XDR_encode_uint32,
      offsetof(struct IPC_Response, ipcref),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_uint32, &XDR_scan_uint32,
      NULL, 0,
      NULL,
      0 },

   { (XDR_Decoder)&XDR_decode_uint32,
      (XDR_Encoder)&XDR_encode_uint32,
      offsetof(struct IPC_Response, result),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_uint32, &XDR_scan_uint32,
      NULL, 0,
      NULL,
      0 },

   { (XDR_Decoder)&XDR_decode_union,
      (XDR_Encoder)&XDR_encode_union,
      offsetof(struct IPC_Response, data),
      NULL, NULL, NULL, 0, 0,
      NULL, NULL,
      &XDR_union_field_deallocator, 0,
      NULL,
      0 },

   { NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL, 0 }
};

static struct XDR_StructDefinition IPC_Response_Struct = {
   IPC_TYPES_RESPONSE, sizeof(struct IPC_Response),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_Response_Fields,
   &XDR_malloc_allocator, &XDR_struct_free_deallocator, &XDR_print_fields_func,
   NULL, NULL
};

static struct XDR_FieldDefinition IPC_ResponseHeader_Fields[] = {
   { (XDR_Decoder)&XDR_decode_uint32,
      (XDR_Encoder)&XDR_encode_uint32,
      offsetof(struct IPC_ResponseHeader, cmd),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_uint32, &XDR_scan_uint32,
      NULL, 0,
      NULL,
      0 },

   { (XDR_Decoder)&XDR_decode_uint32,
      (XDR_Encoder)&XDR_encode_uint32,
      offsetof(struct IPC_ResponseHeader, ipcref),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_uint32, &XDR_scan_uint32,
      NULL, 0,
      NULL,
      0 },

   { (XDR_Decoder)&XDR_decode_uint32,
      (XDR_Encoder)&XDR_encode_uint32,
      offsetof(struct IPC_ResponseHeader, result),
      NULL, NULL, NULL, 0, 0,
      &XDR_print_field_uint32, &XDR_scan_uint32,
      NULL, 0,
      NULL,
      0 },

   { NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL, 0 }
};

static struct XDR_StructDefinition IPC_ResponseHeader_Struct = {
   IPC_TYPES_RESPONSE_HDR, sizeof(struct IPC_ResponseHeader),
   &XDR_struct_encoder, &XDR_struct_decoder, IPC_ResponseHeader_Fields,
   &XDR_malloc_allocator, &XDR_struct_free_deallocator, &XDR_print_fields_func,
   NULL, NULL
};

int IPC_Void_decode(char *src,
      struct IPC_Void *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_Void_Fields);
}

int IPC_Void_encode(
      struct IPC_Void *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TYPES_VOID , IPC_Void_Fields);
}

int IPC_Void_decode_array(char *src,
      struct IPC_Void **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_Void),
            (XDR_Decoder)&IPC_Void_decode, NULL);

   return 0;
}

int IPC_Void_encode_array(
      struct IPC_Void **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_Void),
            (XDR_Encoder)&IPC_Void_encode, NULL);

   return 0;
}

int IPC_Command_decode(char *src,
      struct IPC_Command *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_Command_Fields);
}

int IPC_Command_encode(
      struct IPC_Command *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TYPES_COMMAND , IPC_Command_Fields);
}

int IPC_Command_decode_array(char *src,
      struct IPC_Command **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_Command),
            (XDR_Decoder)&IPC_Command_decode, NULL);

   return 0;
}

int IPC_Command_encode_array(
      struct IPC_Command **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_Command),
            (XDR_Encoder)&IPC_Command_encode, NULL);

   return 0;
}

int IPC_Heartbeat_decode(char *src,
      struct IPC_Heartbeat *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_Heartbeat_Fields);
}

int IPC_Heartbeat_encode(
      struct IPC_Heartbeat *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TYPES_HEARTBEAT , IPC_Heartbeat_Fields);
}

int IPC_Heartbeat_decode_array(char *src,
      struct IPC_Heartbeat **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_Heartbeat),
            (XDR_Decoder)&IPC_Heartbeat_decode, NULL);

   return 0;
}

int IPC_Heartbeat_encode_array(
      struct IPC_Heartbeat **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_Heartbeat),
            (XDR_Encoder)&IPC_Heartbeat_encode, NULL);

   return 0;
}

int IPC_DataReq_decode(char *src,
      struct IPC_DataReq *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_DataReq_Fields);
}

int IPC_DataReq_encode(
      struct IPC_DataReq *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TYPES_DATAREQ , IPC_DataReq_Fields);
}

int IPC_DataReq_decode_array(char *src,
      struct IPC_DataReq **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_DataReq),
            (XDR_Decoder)&IPC_DataReq_decode, NULL);

   return 0;
}

int IPC_DataReq_encode_array(
      struct IPC_DataReq **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_DataReq),
            (XDR_Encoder)&IPC_DataReq_encode, NULL);

   return 0;
}

int IPC_OpaqueStruct_decode(char *src,
      struct IPC_OpaqueStruct *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_OpaqueStruct_Fields);
}

int IPC_OpaqueStruct_encode(
      struct IPC_OpaqueStruct *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TYPES_OPAQUE_STRUCT , IPC_OpaqueStruct_Fields);
}

int IPC_OpaqueStruct_decode_array(char *src,
      struct IPC_OpaqueStruct **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_OpaqueStruct),
            (XDR_Decoder)&IPC_OpaqueStruct_decode, NULL);

   return 0;
}

int IPC_OpaqueStruct_encode_array(
      struct IPC_OpaqueStruct **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_OpaqueStruct),
            (XDR_Encoder)&IPC_OpaqueStruct_encode, NULL);

   return 0;
}

int IPC_OpaqueStructArr_decode(char *src,
      struct IPC_OpaqueStructArr *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_OpaqueStructArr_Fields);
}

int IPC_OpaqueStructArr_encode(
      struct IPC_OpaqueStructArr *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TYPES_OPAQUE_STRUCT_ARR , IPC_OpaqueStructArr_Fields);
}

int IPC_OpaqueStructArr_decode_array(char *src,
      struct IPC_OpaqueStructArr **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_OpaqueStructArr),
            (XDR_Decoder)&IPC_OpaqueStructArr_decode, NULL);

   return 0;
}

int IPC_OpaqueStructArr_encode_array(
      struct IPC_OpaqueStructArr **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_OpaqueStructArr),
            (XDR_Encoder)&IPC_OpaqueStructArr_encode, NULL);

   return 0;
}

int IPC_Response_decode(char *src,
      struct IPC_Response *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_Response_Fields);
}

int IPC_Response_encode(
      struct IPC_Response *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TYPES_RESPONSE , IPC_Response_Fields);
}

int IPC_Response_decode_array(char *src,
      struct IPC_Response **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_Response),
            (XDR_Decoder)&IPC_Response_decode, NULL);

   return 0;
}

int IPC_Response_encode_array(
      struct IPC_Response **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_Response),
            (XDR_Encoder)&IPC_Response_encode, NULL);

   return 0;
}

int IPC_ResponseHeader_decode(char *src,
      struct IPC_ResponseHeader *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, IPC_ResponseHeader_Fields);
}

int IPC_ResponseHeader_encode(
      struct IPC_ResponseHeader *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         IPC_TYPES_RESPONSE_HDR , IPC_ResponseHeader_Fields);
}

int IPC_ResponseHeader_decode_array(char *src,
      struct IPC_ResponseHeader **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_ResponseHeader),
            (XDR_Decoder)&IPC_ResponseHeader_decode, NULL);

   return 0;
}

int IPC_ResponseHeader_encode_array(
      struct IPC_ResponseHeader **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct IPC_ResponseHeader),
            (XDR_Encoder)&IPC_ResponseHeader_encode, NULL);

   return 0;
}

static uint32_t IPC_AUTOCMD_3_types[] = {
   IPC_TYPES_HEARTBEAT, 0
};

static struct CMD_XDRCommandInfo IPC_Commands[] = {
   { IPC_CMDS_STATUS, 0,
     "proc-status",
     "Reports the general health status of the test process",
     NULL,
     NULL, NULL, NULL },
   { IPC_CMDS_DATA_REQ, IPC_TYPES_DATAREQ,
     "proc-data-req",
     "Requests a specific set of telemetry items from the processes",
     NULL,
     NULL, NULL, NULL },
   { 0, IPC_TYPES_DATAREQ,
     "proc-heartbeat",
     "Returns process aliveness status information",
     IPC_AUTOCMD_3_types,
     NULL, NULL, NULL },
   { 0, 0, NULL, NULL, NULL, NULL, NULL, NULL }
};

static struct CMD_ErrorInfo IPC_Errors[] = {
   { IPC_RESULTCODE_SUCCESS, "SUCCESS",
     "No error - success" },
   { IPC_RESULTCODE_INCORRECT_PARAMETER_TYPE, "INCORRECT_PARAMETER_TYPE",
     "Type of command parameter didn't match the expected type" },
   { IPC_RESULTCODE_UNSUPPORTED, "UNSUPPORTED",
     "The target process does not support the command sent" },
   { IPC_RESULTCODE_ALLOCATION_ERR, "ALLOCATION_ERR",
     "Failed to allocate heap memory" },
   { 0, NULL, NULL }
};

static void IPC_structs_constructor()
    __attribute__((constructor));

static void IPC_structs_constructor()
{
   XDR_register_struct(&IPC_Void_Struct);
   XDR_register_struct(&IPC_Command_Struct);
   XDR_register_struct(&IPC_Heartbeat_Struct);
   XDR_register_struct(&IPC_DataReq_Struct);
   XDR_register_struct(&IPC_OpaqueStruct_Struct);
   XDR_register_struct(&IPC_OpaqueStructArr_Struct);
   XDR_register_struct(&IPC_Response_Struct);
   XDR_register_struct(&IPC_ResponseHeader_Struct);
   CMD_register_commands(IPC_Commands, 0);
   CMD_register_errors(IPC_Errors);
}
