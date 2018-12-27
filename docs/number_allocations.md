# Overview
All enumeration, structure, and command numbers need to be globally unique in order for the system to work correctly.  All the numeric values are 4 bytes long.  Numbers are allocated in 1-byte chunks to help minimize coordination overhead.  The high-order byte of all command numbers must be zero.  This table must be updated prior to use any numbers.

# Allocation Table

|Owner     |Use        |Base Number|Description                 |
|----------|-----------|-----------|----------------------------|
|Reserved  |Reserved   |0x00000000 |Reserved and not to be used |
|Built-in  |Commands   |0x00000100 |Internal commands           |
|ADCS      |Commands   |0x00000200 |                            |
|BDOT      |Commands   |0x00000300 |BDOT submodule of ADCS      |
|Built-in  |Structures |0x01000100 |Internal structures         |
|ADCS      |Structures |0x01000200 |                            |
|BDOT      |Structures |0x01000300 |BDOT submodule of ADCS      |
|Built-in  |Errors     |0x02000100 |Internal errors             |
|ADCS      |Errors     |0x02000200 |                            |
|BDOT      |Errors     |0x02000300 |BDOT submodule of ADCS      |

