#pragma once

#include "../types.h"
#include "../iter/iteratorFilter.h"
namespace LightSpeed {

	namespace CompressNumb {

		///Compresses unsigned 64bit number
		/**
		 Compressing of numbers allows to write small numbers by
		 less of bytes. Numbers in the range 0..31 are written into one byte.
		 Numbers in the range 32..8223  are written into two bytes.
		 Numbers in the range 8224..2105375 are written into three bytes etc.

		 @param number to compress
		 @param buffer pointer to buffer. It must be at least 10 bytes length
		 @return count of bytes written into buffer
		*/
		 
		natural compress(Bin::natural64 number, byte *buffer);

		///Decompress unsigned 64bit number
		/**
		 Decompresses buffer into the number
		 @param buffer buffer contains compressed number
		 @return decompressed number

		 To calculate count of bytes needed to decompress, use numberLength() function
		 */
		Bin::natural64 decompress(const byte *buffer);

		///Retrieves count of bytes needed to decompress number
		/**
		@param firstByte first byte of compressed number. 
		@return count of bytes need to decompress the number including the
		first byte.
		*/
		inline natural numberLength( byte firstByte )
		{
			natural fb = firstByte;
			//first 3 bits contains length of number
			return ((fb >> 5) + 1) + ((fb+1)>>8) ;
		}

		inline natural compressSigned(Bin::integer64 number, byte *buffer) {
			if (number >= 0) {
				return compress((Bin::natural64)(number << 1),buffer);
			} else {
				return compress((Bin::natural64)((-number-1) << 1) | 0x1,buffer);
			}
		}

		inline Bin::integer64 decompressSigned(const byte *buffer) {
			Bin::natural64 res = decompress(buffer);
			if (res & 0x1) return -((Bin::integer64)(res >> 1))-1;
			else return (Bin::integer64)(res>>1);
		}

	}

}
