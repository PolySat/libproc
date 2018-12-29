namespace ipc;

const CMD_BASE =       0x100;
const TYPE_BASE = 0x01000100;
const ERR_BASE =  0x02000100;

enum Cmds {
   RESPONSE = CMD_BASE + 0,
   STATUS = CMD_BASE + 1,
   DATA_REQ = CMD_BASE + 2
};

enum types {
   VOID = 0,
   OPAQUE_STRUCT= TYPE_BASE + 1,
   OPAQUE_STRUCT_ARR = TYPE_BASE + 2,
   COMMAND = TYPE_BASE + 3,
   RESPONSE = TYPE_BASE + 4,
   DATAREQ = TYPE_BASE + 5,
   RESPONSE_HDR = TYPE_BASE + 6,
   HEARTBEAT = TYPE_BASE + 7
};

command "proc-status" {
   summary "Reports the general health status of the test process";
} = cmds::STATUS;

command "proc-data-req" {
   summary "Requests a specific set of telemetry items from the processes";
   param types::DATAREQ;
} = cmds::DATA_REQ;

command "proc-heartbeat" {
   summary "Returns process aliveness status information";
   types = types::HEARTBEAT;
};

struct Void {
   void;
} = types::VOID;

union CommandParameters {
};

union Data {
};

struct Command {
   Cmds cmd;
   unsigned int ipcref;
   CommandParameters parameters;
} = types::COMMAND;

struct Heartbeat {
   unsigned hyper commands {
      name "Commands";
      key proc_commands;
      description "The number of commands received by the process";
   };
   unsigned hyper responses {
      name "Responses";
      key proc_responses;
      description "The number of command responses received by the process";
   };
   unsigned hyper heartbeats {
      name "Heartbeats";
      key proc_heartbeats;
      description "The number of heartbeat commands received by the process";
   };
} = types::HEARTBEAT;

enum ResultCode {
   SUCCESS = ERR_BASE + 0,
   INCORRECT_PARAMETER_TYPE = ERR_BASE + 1,
   UNSUPPORTED = ERR_BASE + 2,
   ALLOCATION_ERR = ERR_BASE + 3
};

error ResultCode::SUCCESS = "No error - success";
error ResultCode::INCORRECT_PARAMETER_TYPE = "Type of command parameter didn't match the expected type";
error ResultCode::UNSUPPORTED = "The target process does not support the command sent";
error ResultCode::ALLOCATION_ERR = "Failed to allocate heap memory";

struct DataReq {
   int length;
   types reqs<length> {
      key types;
   };
} = types::DATAREQ;

struct OpaqueStruct {
   int length;
   opaque data<length>;
} = types::OPAQUE_STRUCT;

struct OpaqueStructArr {
   int length;
   OpaqueStruct structs<length>;
} = types::OPAQUE_STRUCT_ARR;

struct Response {
   Cmds cmd;
   unsigned int ipcref;
   ResultCode result;
   Data data;
} = types::RESPONSE;

struct ResponseHeader {
   Cmds cmd;
   unsigned int ipcref;
   ResultCode result;
} = types::RESPONSE_HDR;
