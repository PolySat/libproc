# Motivation
  * [Existing packet creation code contains lots of boilerplate](https://github.com/PolySat/adcs-sensor-reader/blob/13f041b34c37a4a98bd17745b05511e740da0e24/adcs.c#L151)
    * Prone to human error
  * [Existing command generation code contains lots of boilerplate](https://github.com/PolySat/adcs-sensor-reader/blob/13f041b34c37a4a98bd17745b05511e740da0e24/adcs-util.c#L45)
    * Prone to human error
    * Inconsistent between developers
  * [Telemetry archival process contains lots of boilerplate](https://github.com/PolySat/libproc/blob/24bc9c0c6b0a3f6969dc87e6a4009a8259bcebca/docs/dl_config_example.cfg#L1)
    * Time consuming and mind-numbing to write
    * Easily forgotten
  * Command and Telemetry Documentation Lacking
    * Very important, but tedious, to write for each mission
    * Almost identical from mission to mission
  * [API to send a command can't handle responses](https://github.com/PolySat/libproc/blob/2cd7a84476fc8f0b8933bee3847c85b148ae889f/proclib.h#L114)
    * Responses are not even supported in libproc -- everything is a one-way command
  * [Process command configuration file is combersome](https://github.com/PolySat/adcs-sensor-reader/blob/13f041b34c37a4a98bd17745b05511e740da0e24/adcs.cmd.cfg#L1)
    * Subject to typos in both command number and function name
  * libsatpkt is a mess
    * Contains all headers for all processes in a single repo
       * Makes incremental updates a nightmare
       * Painful workflow during early stages of development
  * No consistent method for communicating command errors
  * ADCS needed something more flexible, so no time like the present!

# Big Picture Solution
  * Describe the contents of the packet in a data schema language
    * Ideally include field and command level documentation to make generation of larger document sets easier
  * Use a compiler to produce C code for all the boilerplate / mundane pieces
  * Define a new command / response format in this new language including new commanding APIs

# Tradeoffs
  * Run-time execution overhead increase
    * Marshaling packets is currently very lightweight
    * Any auto-coded solution will result in many more function calls to achieve the same result
      * Mitigated with a table driven back-end, but still more overhead
  * Compiled size increase
    * More functions means larger compiled objects
    * We don't have that much storage on the spacecraft
      * Table driven code can also mitigate, but not eliminate
  * Need to learn another domain specific language before writing processes

# Marshaling / Unmarshaling Code Generation
  * A number of existing options considered
    * [Google Protocol Buffers](https://en.wikipedia.org/wiki/Protocol_Buffers)
    * [Google flatbuffers](https://google.github.io/flatbuffers/)
    * [JSON](https://www.json.org)
  * Over-the-wire packet sizes too large for our 256 byte link
  * Size of libraries to marshal / unmarshal too large for our storage
  * Both largely due to the number of nice-to-have, but not applicable to us, features

# Design Decision - XDR
  * [External Data Representation (XDR)](https://tools.ietf.org/html/rfc4506)
    * Relatively compact data representation (smallest field is 4 bytes, even for a bit)
    * Around since before you were born
    * Used in core protocols (e.g., NFS)
    * Data schema defined by a standard (.x files)
    * A number of freely available .x to your favorite language compilers available
    * Missing some potentially useful data types, like bit fields

# Design Decision - Custom XDR Back-end
  * Existing parsers do not produce table driven back-ends
  * Existing parsers do not generate code that easily integrates with libproc
  * [Added back-end code generation to existing parser written in python](https://github.com/PolySat/polyxdr)
    * Targets libproc
    * Produces table driven code
  * Other language backends can use more standard generated code since they are not flying

# Design Decision - Customized Schema Language
  * Standard XDR.x schema lacked support for key features
    * Documentation
    * Namespaces
    * Imports
  * Breaks compatibility with existing XDR compilers
    * Can always produce .x compatible schema as a compiler backend
    * Forces the use of our custom compiler

# Details - .xp Schema
  * [Example](https://github.com/PolySat/libproc/blob/24bc9c0c6b0a3f6969dc87e6a4009a8259bcebca/cmd-pkt.xp#L1)
  * Each structure has a unique numeric identifier
  * Each command has a unique numeric identifier
  * Each error has a unique numeric identifier
  * [Numbers are globally unique](https://github.com/PolySat/libproc/blob/24bc9c0c6b0a3f6969dc87e6a4009a8259bcebca/docs/number_allocations.md)
  * Each field in the structure has optional documentation
  * Supported types
    * int
    * unsigned int
    * hyper
    * unsigned hyper
    * float
    * double
    * string
    * enums
    * structs
    * variable length arrays

# Details - Backend Code
  * [generated .c file](https://github.com/PolySat/libproc/blob/6073b303e23f7205ee88e349b0cf87534107dd42/docs/cmd-pkt.c#L1)
  * [generated .h file](https://github.com/PolySat/libproc/blob/6073b303e23f7205ee88e349b0cf87534107dd42/docs/cmd-pkt.h#L1)
    * Note how namespaces translate into struct names since C does not support native namespaces
  * Functions for intrinsic data types are in [libproc](https://github.com/PolySat/libproc/blob/6073b303e23f7205ee88e349b0cf87534107dd42/xdr.c#L1), not the generated code

# Details - Packaging
  * The generated .c file is intended to be compiled into an independent .so (shared library) file and installed into /usr/lib
  * The headers should all install into /usr/include/polypkt

# API - Command Line Utility
  * Previously a very repetitive task to write -util programs
  * Now 99.9999% is generated from the schema
  * [Example new boilerplate util code](https://github.com/PolySat/example-process/blob/406921dbe18755fbec190735881532238b6212fd/example-util.c#L1)
  * Demo of the util program running

# API - Implementing commands with side effects
  * Create an array of handler functions
  * Create the handler functions
  * Pass the array in to PROC_init
  * Handle the command
  * Use either IPC_response or IPC_error to send result back to requester
  * [Example](https://github.com/PolySat/example-process/blob/406921dbe18755fbec190735881532238b6212fd/payload.c#L13)

# API - Commands that only return data
  * Create "populator" function
  * Register using [XDR_register_populator](https://github.com/PolySat/libproc/blob/6073b303e23f7205ee88e349b0cf87534107dd42/xdr.h#L70) function call
  * Function passes the filled-in structure back via a function pointer provided as an input parameter
  * [Example](https://github.com/PolySat/example-process/blob/406921dbe18755fbec190735881532238b6212fd/payload.c#L37)

# API - Sending commands to other processes
  * New IPC_comand function
    * [Declaration](https://github.com/PolySat/libproc/blob/6073b303e23f7205ee88e349b0cf87534107dd42/ipc.h#L298)
    * [Usage](https://github.com/PolySat/libproc/blob/e9d93fb7b50d53383cce4345b5a9a59a397fdd8a/programs/xdr/util-proc.c#L61)
      * Notice the flexible response callback!

# Protocol Details
  * Schema taken from [cmd-pkt.xp](https://github.com/PolySat/libproc/blob/24bc9c0c6b0a3f6969dc87e6a4009a8259bcebca/cmd-pkt.xp#L1)
  * Command header
~~~~
enum Cmds {
   RESPONSE = CMD_BASE + 0,
   STATUS = CMD_BASE + 1,
   DATA_REQ = CMD_BASE + 2
};

union CommandParameters {
};

struct Command {
   Cmds cmd;
   unsigned int ipcref;
   CommandParameters parameters;
} = types::COMMAND;
~~~~
   * Command number (enumerated value)
   * Requester reference number
   * Command parameters is a union of all possible structures
     * XDR encodes unions by first encoding the 4 byte type number then the actual structure 
     * Therefore all structures need to given unique numbers
     * See the `} = types::COMMAND'` syntax
  * Response header
~~~~
enum ResultCode {
   SUCCESS = ERR_BASE + 0,
   INCORRECT_PARAMETER_TYPE = ERR_BASE + 1,
   UNSUPPORTED = ERR_BASE + 2,
   ALLOCATION_ERR = ERR_BASE + 3
};

struct Response {
   Cmds cmd;
   unsigned int ipcref;
   ResultCode result;
   Data data;
} = types::RESPONSE;
~~~~
  * Libproc error codes (I'm sure this list will grow over time)
~~~~
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
~~~~
  * Libproc built-in commands
~~~~
command "proc-status" {
   summary "Reports the general health status of the test process";
} = cmds::STATUS;

struct DataReq {
   int length;
   types reqs<length> {
      key types;
   };
} = types::DATAREQ;

command "proc-data-req" {
   summary "Requests a specific set of telemetry items from the processes";
   param types::DATAREQ;
} = cmds::DATA_REQ;

command "proc-heartbeat" {
   summary "Returns process aliveness status information";
   types = types::HEARTBEAT;
};
~~~~
  * Libproc built-in heartbeat structure
~~~~
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
~~~~
  * Support for results that contain more than one data type (e.g., array of unions)
~~~~
struct OpaqueStruct {
   int length;
   opaque data<length>;
} = types::OPAQUE_STRUCT;

struct OpaqueStructArr {
   int length;
   OpaqueStruct structs<length>;
} = types::OPAQUE_STRUCT_ARR;
~~~~