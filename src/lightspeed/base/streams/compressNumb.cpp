#include <typeinfo>
#include "../types.h"
#include "compressNumb.h"
#include "../exceptions/throws.tcc"
namespace LightSpeed {

	
	static inline natural calcCompressedLen(Bin::natural64 n) {
		if (n < 0x20LL) return 0;
		if (n < 0x2020LL) return 1;
		if (n < 0x202020LL) return 2;
		if (n < 0x20202020LL) return 3;
		if (n < 0x2020202020LL) return 4;
		if (n < 0x202020202020LL) return 5;
		if (n < 0x20202020202020LL) return 6;
		if (n < 0x1F20202020202020LL) return 7;
		return 8;
	}

	static Bin::natural64 compressOffsets[9] = {
			0LL,
			0x20LL,
			0x2020LL,
			0x202020LL,
			0x20202020LL,
			0x2020202020LL,
			0x202020202020LL,
			0x20202020202020LL,
			0LL
	};

	natural CompressNumb::compress( Bin::natural64 number, byte *buffer )
	{
		natural len = calcCompressedLen(number);
		byte firstByte;

		if (len == 8) {
			firstByte = 0xFF;
		} else {
			firstByte = (byte)(len << 5);
		}
		number-= compressOffsets[len];
		natural res = len + 1;
		while (len > 0) {
			buffer[len] = (byte)(number & 0xFF);
			len--;
			number >>= 8;
		}
		buffer[0] = (byte)(number & 0xFF) | firstByte;
		return res;
	}

	Bin::natural64 CompressNumb::decompress( const byte *buffer )
	{
		Bin::natural64 res = 0;
		byte x = *buffer;
		natural len = numberLength(x) - 1;
		
		///for len < 8, mask 0x1F, for len == 8, reset number
		x &= (0x1F >> (len & 0x8));
		
		natural i = 0;
		while (i < len) {
			res = (res << 8) + x;
			i++;
			x = buffer[i];
		}
		res = (res << 8) + x;
		res+=compressOffsets[len];
		return res;			
	}

}
