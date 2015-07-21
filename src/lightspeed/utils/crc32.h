#pragma once

#include "../base/types.h"

namespace LightSpeed {

class Crc32
{
public:
    Crc32() { reset(); }
    ~Crc32() throw() {}
    void reset() { _crc = (Bin::natural32)~0; }
    void blockWrite(const byte* pData, natural length)
    {
    	const byte* pCur = pData;
        natural remaining = length;
        for (; remaining--; ++pCur)
            _crc = ( _crc >> 8 ) ^ kCrc32Table[(_crc ^ *pCur) & 0xff];
    }

    void write(byte b) {
        _crc = ( _crc >> 8 ) ^ kCrc32Table[(_crc ^ b) & 0xff];
    }

    Bin::natural32 getCrc32() { return ~_crc; }

private:
    Bin::natural32 _crc;

    static const Bin::natural32 kCrc32Table[256];
};

}
