#ifndef CMD_PKT_H
#define CMD_PKT_H

#include "xdr.h"
#include "cmd.h"
#include <stdint.h>

#define IPC_CMD_BASE (256)

#define IPC_TYPE_BASE (16777472)

#define IPC_ERR_BASE (33554688)

enum IPC_CMDS {
   IPC_CMDS_RESPONSE = IPC_CMD_BASE + 0,
   IPC_CMDS_STATUS = IPC_CMD_BASE + 1,
   IPC_CMDS_DATA_REQ = IPC_CMD_BASE + 2,
};

enum IPC_TYPES {
   IPC_TYPES_VOID = 0,
   IPC_TYPES_OPAQUE_STRUCT = IPC_TYPE_BASE + 1,
   IPC_TYPES_OPAQUE_STRUCT_ARR = IPC_TYPE_BASE + 2,
   IPC_TYPES_COMMAND = IPC_TYPE_BASE + 3,
   IPC_TYPES_RESPONSE = IPC_TYPE_BASE + 4,
   IPC_TYPES_DATAREQ = IPC_TYPE_BASE + 5,
   IPC_TYPES_RESPONSE_HDR = IPC_TYPE_BASE + 6,
   IPC_TYPES_HEARTBEAT = IPC_TYPE_BASE + 7,
};

enum IPC_RESULTCODE {
   IPC_RESULTCODE_SUCCESS = IPC_ERR_BASE + 0,
   IPC_RESULTCODE_INCORRECT_PARAMETER_TYPE = IPC_ERR_BASE + 1,
   IPC_RESULTCODE_UNSUPPORTED = IPC_ERR_BASE + 2,
   IPC_RESULTCODE_ALLOCATION_ERR = IPC_ERR_BASE + 3,
};

struct IPC_Void {
   int voidfield;
};

struct IPC_Command {
   uint32_t cmd;
   uint32_t ipcref;
   struct XDR_Union parameters;
};

struct IPC_Heartbeat {
   uint64_t commands;
   uint64_t responses;
   uint64_t heartbeats;
};

struct IPC_DataReq {
   int32_t length;
   uint32_t *reqs;
};

struct IPC_OpaqueStruct {
   int32_t length;
   char *data;
};

struct IPC_OpaqueStructArr {
   int32_t length;
   struct IPC_OpaqueStruct *structs;
};

struct IPC_Response {
   uint32_t cmd;
   uint32_t ipcref;
   uint32_t result;
   struct XDR_Union data;
};

struct IPC_ResponseHeader {
   uint32_t cmd;
   uint32_t ipcref;
   uint32_t result;
};

extern int IPC_Void_decode(char *src,
      struct IPC_Void *dst, size_t *used,
      size_t max, void *len);
extern int IPC_Void_encode(
      struct IPC_Void *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_Void_decode_array(char *src,
      struct IPC_Void **dst, size_t *used,
      size_t max, void *len);
extern int IPC_Void_encode_array(
      struct IPC_Void **src, char *dst, size_t *used,
      size_t max, void *len);

extern int IPC_Command_decode(char *src,
      struct IPC_Command *dst, size_t *used,
      size_t max, void *len);
extern int IPC_Command_encode(
      struct IPC_Command *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_Command_decode_array(char *src,
      struct IPC_Command **dst, size_t *used,
      size_t max, void *len);
extern int IPC_Command_encode_array(
      struct IPC_Command **src, char *dst, size_t *used,
      size_t max, void *len);

extern int IPC_Heartbeat_decode(char *src,
      struct IPC_Heartbeat *dst, size_t *used,
      size_t max, void *len);
extern int IPC_Heartbeat_encode(
      struct IPC_Heartbeat *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_Heartbeat_decode_array(char *src,
      struct IPC_Heartbeat **dst, size_t *used,
      size_t max, void *len);
extern int IPC_Heartbeat_encode_array(
      struct IPC_Heartbeat **src, char *dst, size_t *used,
      size_t max, void *len);

extern int IPC_DataReq_decode(char *src,
      struct IPC_DataReq *dst, size_t *used,
      size_t max, void *len);
extern int IPC_DataReq_encode(
      struct IPC_DataReq *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_DataReq_decode_array(char *src,
      struct IPC_DataReq **dst, size_t *used,
      size_t max, void *len);
extern int IPC_DataReq_encode_array(
      struct IPC_DataReq **src, char *dst, size_t *used,
      size_t max, void *len);

extern int IPC_OpaqueStruct_decode(char *src,
      struct IPC_OpaqueStruct *dst, size_t *used,
      size_t max, void *len);
extern int IPC_OpaqueStruct_encode(
      struct IPC_OpaqueStruct *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_OpaqueStruct_decode_array(char *src,
      struct IPC_OpaqueStruct **dst, size_t *used,
      size_t max, void *len);
extern int IPC_OpaqueStruct_encode_array(
      struct IPC_OpaqueStruct **src, char *dst, size_t *used,
      size_t max, void *len);

extern int IPC_OpaqueStructArr_decode(char *src,
      struct IPC_OpaqueStructArr *dst, size_t *used,
      size_t max, void *len);
extern int IPC_OpaqueStructArr_encode(
      struct IPC_OpaqueStructArr *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_OpaqueStructArr_decode_array(char *src,
      struct IPC_OpaqueStructArr **dst, size_t *used,
      size_t max, void *len);
extern int IPC_OpaqueStructArr_encode_array(
      struct IPC_OpaqueStructArr **src, char *dst, size_t *used,
      size_t max, void *len);

extern int IPC_Response_decode(char *src,
      struct IPC_Response *dst, size_t *used,
      size_t max, void *len);
extern int IPC_Response_encode(
      struct IPC_Response *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_Response_decode_array(char *src,
      struct IPC_Response **dst, size_t *used,
      size_t max, void *len);
extern int IPC_Response_encode_array(
      struct IPC_Response **src, char *dst, size_t *used,
      size_t max, void *len);

extern int IPC_ResponseHeader_decode(char *src,
      struct IPC_ResponseHeader *dst, size_t *used,
      size_t max, void *len);
extern int IPC_ResponseHeader_encode(
      struct IPC_ResponseHeader *src, char *dst, size_t *used,
      size_t max, void *len);
extern int IPC_ResponseHeader_decode_array(char *src,
      struct IPC_ResponseHeader **dst, size_t *used,
      size_t max, void *len);
extern int IPC_ResponseHeader_encode_array(
      struct IPC_ResponseHeader **src, char *dst, size_t *used,
      size_t max, void *len);

extern void IPC_forcelink(void);

#endif
