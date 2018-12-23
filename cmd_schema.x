namespace ipc;

const BASE = 0x100;

enum Commands {
   STATUS = BASE + 1,
   DATA_REQ = BASE + 2
};

enum data_types {
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
} = commands::STATUS;

command "proc-data-req" {
   summary "Requests a specific set of telemetry items from the processes";
} = commands::DATA_REQ;

struct Void {
   void;
} = data_types::VOID;

union CommandParameters {
};

union Data {
};

struct Command {
   Commands cmd;
   unsigned int ipcref;
   CommandParameters parameters;
} = data_types::COMMAND;

enum ResultCode {
   SUCCESS = BASE + 0,
   INCORRECT_PARAMETER_TYPE = BASE + 1
};

error ResultCode::SUCCESS = "No error - success";
error ResultCode::INCORRECT_PARAMETER_TYPE = "Type of command parameter didn't match the expected type";

struct DataReq {
   int length;
   data_types reqs<length>;
} = data_types::DATAREQ;

struct OpaqueStruct {
   int length;
   opaque data<length>;
} = data_types::OPAQUE_STRUCT;

struct OpaqueStructArr {
   int length;
   OpaqueStruct structs<length>;
} = data_types::OPAQUE_STRUCT_ARR;

struct Response {
   unsigned int ipcref;
   ResultCode result;
   Data data;
} = data_types::RESPONSE;

struct ResponseHeader {
   unsigned int ipcref;
   ResultCode result;
} = data_types::RESPONSE_HDR;
