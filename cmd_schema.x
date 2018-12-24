namespace ipc;

const BASE = 0x100;

enum Cmds {
   STATUS = BASE + 1,
   DATA_REQ = BASE + 2,
   RESPONSE = BASE + 3
};

enum types {
   VOID = 0,
   OPAQUE_STRUCT= BASE + 1,
   OPAQUE_STRUCT_ARR = BASE + 2,
   COMMAND = BASE + 3,
   RESPONSE = BASE + 4,
   DATAREQ = BASE + 5,
   RESPONSE_HDR = BASE + 6
};

command "proc-status" {
   summary "Reports the general health status of the test process";
} = cmds::STATUS;

command "proc-data-req" {
   summary "Requests a specific set of telemetry items from the processes";
} = cmds::DATA_REQ;

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

enum ResultCode {
   SUCCESS = BASE + 0,
   INCORRECT_PARAMETER_TYPE = BASE + 1,
   UNSUPPORTED = BASE + 2
};

error ResultCode::SUCCESS = "No error - success";
error ResultCode::INCORRECT_PARAMETER_TYPE = "Type of command parameter didn't match the expected type";
error ResultCode::UNSUPPORTED = "The target process does not support the command sent";

struct DataReq {
   int length;
   types reqs<length>;
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
