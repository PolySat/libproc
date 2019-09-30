# Communication
libproc includes a low-ish overhead communication protocol that attempts to reduce the time and busywork required of developers.  The communication protocol is based on [XDR (External Data Representation Standard)](https://tools.ietf.org/html/rfc4506).  There are minimal changes to the over-the-wire protocol and more changes to the XDR schema syntax, each of which is described in the following sections.

# Schema
The objective of the schema is to streamline the developer's time when defining additional command formats.  The schema is a modified version of the XDR schema.  Most of the additions focus on adding support for consolid command and data documentation, but there are also additions to help  the format scale to a more distributed architecture, such as namespaces and include directives.  There is a custom, python-based parser in the xdrgen folder of libproc.

## Schema Narrative

### namespace
The schema must start with a namespace declaration:
```
namespace ID;
```
The root namespace is assumed to be `IPC`.  All other namespaces fall under IPC.  Sub-namespaces can exist via imports.  Namespaces in imported files are added under the namespace of their parent file.

### Imports
The import directive can be used at any time at the top-level of the schema file.  Importing another schema file is an easy way to delegate parts of the schema to sub-modules within the same process.  All imported items are incorporated into the root schema in a sub-namespace.  The format of the import statement is:
```
import "relative path to .x file";
```
### Constants
Constants are a good way ensure all command, structure, and error numbers come from a consistent number space.  They get converted into the appropriate constant type of the target language.  The syntax for constants is:
```
const BASE = VALUE;
```
The VALUE can be expressed in either decimal or hexadecimal.

### Enumerations
Enumerations are used for multiple purposes.  First, you must use enumerated types as the basis for type numbers, command numbers, and error numbers.  You should also use them when storing enumerated types in a structure.  The syntax for enumerations is:
```
enum NAME {
   ITEM1 = VALUE1;
   ITEM2 = VALUE2;
};
```
A value can either be a constant, numeric literal, or a constant plus a numeric literal.

### Errors
Error values get assigned a textual description.  This assists in documentation and auto-generating code that produces more human readable error messages.  The syntax for errors is:
```
error ENUM_NAME::ENUM_ITEM = "error message";
```
Note that all error numbers must be defined as an enumerated type.  You must specify both the name of the enumerated type and the name of the item when specifying the error number.

### Commands
Commands specify the actual commands sent over the network to control the target process.  There are two general types of commands.  First, you can specify commands that can be handled by a callback function in the process.  This can also return data via a response.  The second type of command is one that only returns data without side effects.  This type of command is so prevalent that there is special handling code in libproc to streamline it and make it more flexible.  The syntax for commands is:
```
command "command-name" {
   summary "command description";
   param ENUM_NAME::ENUM_ITEM;
   types = ENUM_NAME::ENUM_ITEM, ENUM_NAME::ENUM_ITEM, ...;
} = ENUM_NAME::ENUM_ITEM;
```
As with other numbers, all command numbers must be defined as an enumerated type.  Enumerated type names must include both the enumeration name and the item name, separated by double colons.  The summary field is required but the other fields are only needed some of the time.

`summary`: A textual description of what the command does.  This description flows down to command-line help messages and other documentation.

`params`: If present it specifies the structure number of the parameters to the command.  The structure must be defined in a structure statement (see the next section).  Commands that do not require parameters may omit this option and will automatically be given a void param.

`types`: A comma separated list of structure numbers to request for this command.  This triggers the special handling code that automatically responds with the correct structures.  When using they types option you do not specify a command number.  The list of types gets set to the remote process using a built-in command.

`...} = ENUM_NAME::ENUM_ITEM`: This option specifies the number of this command.  It is mutually exclusive with the `types` field.

### Structures
Structures define the data that gets sent over the network.  They result in an equivalent language-level structure and code that automatically encodes it and decodes it for transmission.  They also include field-level documentation and identifiers that are included in generated documentation and command line utilities.  The syntax for structure definitions is:
```
struct Name {
   fieldtype fieldname {
      name "Name";
      key identifier;
      description "Field description";
      unit "Unit";
      divisor Value;
      offset Value;
   };
   int arrlen;
   fieldtype fieldname[arrlen];
   ...
} = ENUM_NAME::ENUM_ITEM;
```
Structure names must be unique within the namespace.  They must have one or more field definitions.  The field documentation appears in the `{}` block following the fieldname and before the semi-colon.  It is optional but strongly encouraged.  In some cases, just as a length field for an array, require no documentation.

`fieldtype`: The type of the structure field.  Available types are: `int`, `unsigned int`, `hyper`, `unsigned hyper`, `float`, `string`, `opaque`, and any enumeration or structure name.

When specifying an array you must separately declare a field to hold the array length and it must be before the array in the structure.  Use the `[]` syntax to specify a field as an array and denote the field that serves as the length.

`...} = ENUM_NAME::ENUM_ITEM`: This option specifies the number of this structure.

#### Field Documentation Options
The following options are available for fields.  These options can be specified in any order:

`name`: Provides a textual, human-readable name for this field.  This is used in documentation and when printing the field to stdout in human-readable form.

`type`: A globally unique identifier for this field.  It isn't a quoted string and needs to follow standard language identifier rules.  The key needs to be globally unique among all fields in the entire combined schema.  The key is used when producing key=value pair output and comma separated output.  It is also used to identify the field when automatically parsing from the command line.  If this field is omitted the field will not be available for printing or scanning from a command line.

`description`: A longer description of this field.  It is used for documentation and command line help.

`unit`: The unit label for the engineering units of the field.

`divisor`: `offset`: Used to specify a linear conversion from raw units to engineering units.  The raw value is converted as follows: `(value/divisor) + offset`.  The values in the structure are always transmitted as raw values.  If no values are provided 0 is assumed.  A 0 divisor results in no conversion, not a math error.

### Number spaces
All enumeration, structure, and command numbers need to be globally unique in order for the system to work correctly.  All the numeric values are 4 bytes long.  Numbers are allocated in 1-byte chunks to help minimize coordination overhead.  The high-order byte of all command numbers must be zero.  The number allocations are  
documented in the [libproc number allocation table](https://github.com/PolySat/libproc/blob/xdr-updates/docs/number_allocations.md).  This table must be updated prior to use any numbers.

## Example
The following example shows a small schema, including structures, commands, and errors.
```
namespace example;

const CMD_BASE =    0xFFFF00;
const TYPE_BASE = 0x01FFFF00;
const ERR_BASE =  0x02FFFF00;

enum Cmds {
   SET_SPEED = CMD_BASE + 0

};

enum Types {
   SPEED_SETTINGS = TYPE_BASE + 0,
   DEBUG = TYPE_BASE + 1
};

enum Responses {
   BAD_SPEED = TYPE_BASE + 0
};


error Responses::BAD_SPEED = "User specified an unsupported speed";

struct Speed {
   float x {
      name "X Speed";
      key xspeed;
      description "The rotational speed of the x-axis wheel";
      unit "RPM";
   };
   float y {
      name "Y Speed";
      key yspeed;
      description "The rotational speed of the y-axis wheel";
      unit "RPM";
   };
   float z {
      name "Z Speed";
      key zspeed;
      description "The rotational speed of the z-axis wheel";
      unit "RPM";
   };
};

struct Debug {
   unsigned int state {
      name "State";
      key wheel_state;
      description "The state of the wheel driver.  Provides a bit-field of internal information.";
   };
};

command "example-set-wheel-speed" {
   summary "Sets the speed of the reaction wheels";
   param types::Speed;
} = cmds::SET_SPEED;

command "example-debug" {
   summary "Provides debugging data about the wheel sub-system";
   types = types::DEBUG;
};

```

## Built-in Functionality
libproc includes some built-in functionality to support universal features across all programs.  The include providing generic structures for commands, responses, common error messages, and process heartbeats.  All the constants, structures, and related functions can be found in libproc's "cmd_schema.h" header file.

### Heartbeat Structure
Heartbeats can be requested via the `IPC::types::HEARTBEAT` structure and `IPC::cmds::DATA_REQ` command.  It contains basic command, response, and heartbeat counters.

### Errors
libproc provides 4 error messages, including one that denotes success:
```
error ResultCode::SUCCESS = "No error - success";
error ResultCode::INCORRECT_PARAMETER_TYPE = "Type of command parameter didn't match the expected type";
error ResultCode::UNSUPPORTED = "The target process does not support the command sent";
error ResultCode::ALLOCATION_ERR = "Failed to allocate heap memory";
```
### Structures
The built-in structures include the following:

`struct Heartbeat`: The data returned in the heartbeat query.
`struct Command`: The generic structure for a command, including the command number, identification value, and any parameters.
`struct Response`: The generic structure for a command response, including identification value and any response values.
`struct DataReq`: The parameters to the data request command that allows the requesting of data by structure number.

### Commands
The built-in commands include the following:
`IPC::cmds::DATA_REQ`: The command to request data structures by number.
`IPC::types::HEARTBEAT`: The command to request a heartbeat from the process.


# Parser
The parser and code generator is written in Python.  It can be found in xdrgen/xdrgen within the libproc project.  To use the parser run the following:
```
xdrgen/xdrgen --target libproc --output <prefix> cmd_schema.x
```
where <prefix> is the base name of the output file you want.  This will generate both a <prefix>.c and <prefix>.h file.

# C API

# Over-the-wire Protocol

# Marshaling / Unmarshaling API

# Overhead
