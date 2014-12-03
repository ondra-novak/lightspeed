/*
 * bsjson.h
 *
 *  Created on: 13.4.2014
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_JSON_BSJSON_H_
#define LIGHTSPEED_UTILS_JSON_BSJSON_H_

/*
 * Binary serialized JSON
 *
 * <opcode><payload>
 * <opcode><size><payload>
 *
 * Compressed numbers
 * 1. pick number
 * 2. represent it as UNICODE character
 * 3. store in UTF-8
 * 4. you can store numbers 0 - 0x10FFFF for standard UTF-8 set,
 *                    or 0 - 0x7FFFFFFF for extended and currently unsupported UTF-8 set
 *                    but this set is recognized inside BSJSON for compressed numbers
 *
 * opcodes are always compressed
 *
 * Opcode sets
 * 0x00 - 0x0F     control set
 * 0x10 - 0x1F     integer
 * 0x20 - 0x2F     float
 * 0x30 - 0x3F     string
 * 0x40 - 0x4F     array
 * 0x50 - 0x5F     object
 * 0x60 - 0x6F     binary data
 * 0x70 - 0x7F     reserved
 * 0x80 - ...	   user defined opcodes - because compression, user defined opcodes has always
 *                 at least 2 bytes. So it is not efficient to define user opcodes for
 *                 values which has less than 3 bytes total.
 *
 *
 * Chunks:
 *
 * If there are more then 14 bytes (items], they are stored in chunk. First chunk has always 15 bytes (items)
 * and is followed by compressed number which specifies count of remaining bytes (items). Second chunk
 * don't need to contain all remaining data, multiple chunks are allowed. Chunked chain is terminated
 * by zero chunk. Example:
 *
 * <opcode 0x?F> <15-items> <size-of-next-chunk> <data> <size-of-next-chunk> <data>...... <00>
 *
 *
 *
 *
 * 0X control set:
 * 00    - null
 * 01    - true
 * 02    - false
 * 03    - push   - push value into internal stack - it can be later poped (see using stack)
 * 04    - pop (fifo) - pop first inserted value
 * 05    - pop (lifo) - pop last inserted value
 * 06    - dup    - duplicate Nth item (N follows in stream). Items are counted from back. 06 00 duplicates last item
 * 07    - unused
 * 08    - unused
 * 09    - unused
 * 0A    - reset checksum (see below)
 * 0B    - checksum (see below)
 * 0C    - clear stack - removes all values from the stack
 * 0D    - NOP (dead) - no operation
 * 0E    - reset - resets parser  - clear stack and removes all definitions
 * 0F    - define opcode
 *               this opcode allows to assign any following value to am opcode
 *               later, when the opcode appears in the stream, it is replaced
 *               by predefined value. In most of cases, this will be used
 *               to replace names of fields. But this opcode allows to
 *               store any value, including arrays and objects. Value can
 *               also contain user defined opcodes, which should be evaluated
 *               when they are used. Value can also contain stack operations
 *
 *               0F <opcode> <value>
 *
 *               Opcode must be above 0x7F
 *
 * 1X set (integers):
 *
 * following bytes are read as signed integer in little endian order. X in the opcode
 * specifies count of bytes. This allows to store integer from 1 to 15 bytes. Actually
 * currently only 0 - 8 bytes are supported. If you wish to store 8 bit of unsigned
 * number, use 0x19 xxxxxxxxxxxxxxxx00 sequence.
 *
 * 2x set (floats):
 *
 * following bytes are read as float number. These opcodes are supported
 *
 * 21  Half (IEEE 754-2008) - 2 bytes float
 * 23  Single 				- 4 bytes float
 * 25  Pascal               - 6 bytes float (real)
 * 27  Double               - 8 bytes float
 * 29  Extended             - 10 bytes float
 * 2F  Quad					- 16 bytes float
 *
 * See: http://en.wikipedia.org/wiki/Floating_point#Internal_representation
 *
 * 3x set (strings):
 *
 * following bytes are read as string. String is stored in UTF-8 charset.
 * For X in range 0x0 - 0xE, X bytes are read to store as string.
 * For X = 0xF, 15 bytes are read followed by another compressed number that specifies size
 * of next string "chunk". Zero chunk is interprted as end of string
 *
 * 30             - empty string
 * 31 0x41        - string "A"
 * 35 "Hello"       - string "Hello" (6 bytes total)
 * 3F "This is very lo" 28 "ng string which will be stored in chunks" 00
 *
 * There is no special rule about size of each chunk. Only first chunk have to be 15 bytes long. Each
 * other chunk can have at least 1 bytes. There is no upper limit. In most of cases, long strings will
 * be stored in three chunks: first 15 bytes, followed by rest of string and followed by zero chunk.
 *
 * 4x set (arrays):
 *
 * This opcode defines array.
 *
 * X specifies count of items. Similar to strings, arrays can be also stored chunked. For 15 and more items
 * in the array, additional chunks may be appended, where zero chunk is interpreted as end of array
 *
 * 40  - empty array
 * 43 11 01 11 02 11 03  - [1,2,3]
 * 4F <15 items> 09 <9 items> 00 - [ 24 items array ]
 *
 * 5x set (objects):
 *
 * This opcode defined object
 *
 * Object is stored similar as array, with exception that each item is stored as pair of values. The first
 * value have to be always string, other value can be anything
 *
 * 50  - empty object
 * 51 31 41 11 01  - {"A":1}
 * 52 31 41 11 01 31 42 01 - {"A":1,"B":true}
 *
 * For more items then 14, chunks are used similar to array
 *
 * 6x set (binary):
 *
 * This opcode is used to store a binary data. This has no equivalent in JSON, nearest is string stored
 * in BASE64. Using this opcode, you don't need to store binary data as BASE64.
 *
 * Opcode 6x is defined same way as opcode 3x, in exception that no codepage is applied.
 *
 * 7x set (reserved)
 *
 * 7x set is reserved for future expansions. Opcodes should not be redefined (see special note)
 *
 * Defining of the opcodes:
 * ------------------------
 *
 * Correct way is use opcodes 0x80 and above for user defined values. Specification also says, that redefinition
 * of any build-in is also valid operation but it has following consequences. Once the command 0F redefines any
 * existing build-in operation, there is no way how to restore its original state unless command 0E is issued.
 * Opcodes 00 - 0F cannot be redefined in any way.
 *
 *
 * Checksum (0A)
 * --------------------
 *
 * Checksum is basic protection of the format against unwanted changes caused by transfer errors
 * or memory corruption. CRC32 is used. Operation Reset checksum (0A) sets checksum to initial state.
 * Operation checksum (0B) checks, whether current checksum contains correct value.
 *
 * <0A> .... data ..... <0B> <XX> <YY>
 *
 * After opcode <0B>, there are two bytes in little endian order that contains lowest 16 bits of current
 * CRC32 value. If current CRC32 doesn't match specified value, parser rejects whole stream as invalid (discarding
 * already parsed content). These two bytes are not included into CRC (but opcode itself is included).
 *
 * Code 0B can be repeated, in this case, bytes XX and YY of previous checksum are also included into CRC32.
 *
 * <0A> [[ ...... data ..... <0B> ]  <XX> <YY> ......... data .......  <0B> ] <xx> <yy>
 *
 * Braces shows which bytes are included into CRC32 calculation
 *
 *
 * Stack:
 * ----------
 *
 * Parser can benefit from the stack. Stack can be used to reduce size of stream if it is used to transfer
 * many instances of a signle object - such a tables. In conjuction with opcode 0x0F, format can define
 * structure of one row in the table, where pop-opcodes are used instead values. After definition of the row,
 * format can contain several push operations followed by opcode of predefined row which is evaluated by poping
 * back these values and creating required structure as result. Example
 *
 *  [ { "Name":"John Smith", "Age":40, "Sex":"M" }, {"Name":"Jenny Smith", "Age":35, "Sex":"F"},....]
 *
 *  0F C2 01 53 34 "Name" 04 33 "Age" 04 33 "Sex" 04  - row format
 *  4F - chunked array
 *  03 5A "John Smith" 03 11 28 03 51 "M" C2 01 - first row
 *  03 5B "Jenny Smith" 03 11 23 03 51 "F" C2 01 - second row
 *  ... etc ....
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */



#pragma once

namespace LightSpeed {



}


#endif /* LIGHTSPEED_UTILS_JSON_BSJSON_H_ */
