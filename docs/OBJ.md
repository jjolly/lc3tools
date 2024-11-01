# LC3Tools Object File Format
This document provides the object file format used by LC3Tools. This is a binary file format that consists of one header and several chunks each which representsone 16-bit value. The first should be the .ORIG value of the orignal source, and subsequent values should be data or instructions.

## File Format
### Header
* 5 bytes : File Magic - `x1C x30 x15 xC0 x01`
* 2 bytes : Version - `x01 x01`

### Data Value
* 2 bytes : Value
* 1 byte  : Origin Flag - `x00` == Not Origin, `x01` == Is Origin
* 4 bytes : Number of Characters to Read from Following String
* n bytes : Value String (length from number of characters)

### Object File
* 1 Header
* 1 Data Value
  * Origin Flag == `x01`
* n Data Values, one for each 16-bit value in memory, starting from the origin
  * Origin Flag == `x00`

## Notes
Different parts of the program are represented in different ways:

* `.ORIG`: This must be the first data value after the 7-byte header. This is the only data value that should have the Origin Flag set to `x01`. This will be the value assigned to the PC when the program is loaded.
* Instructions: Each instruction is the 16-bit value after being assembled. The actual line of code should be included as the string in the data value, without any comments.
* `.FILL`: Single 16-bit values are stored as expected.
* `.BLKW`: Each 16-bit value is represented as a zero value. The entire line of code for the `.BLKW` is part of each data value.
* `.STRINGZ`: Each 16-bit value is one character of the string. The string of the data value is the same as the data value itself.
* `.END`: Not represented in the file. When the last data value is read from the file, then the end marker is assumed.
* Comments: Are not included in any data value strings.
* Traps: Predefined trap instructions are given as they were written. They are not shown as `TRAP` instructions with numeric arguments.

The data order of multibyte integers (16-bit values, 32-bit string lengths) is determined by the build architecture. This means object files from little-endian systems will fail to load on big-endian systems.
