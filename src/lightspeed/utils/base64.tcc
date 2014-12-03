/*
 * base64.cpp
 *
 *  Created on: Oct 15, 2012
 *      Author: ondra
 */




#include "base64.h"
#include "../base/types.h"
#include "../base/iter/iteratorFilter.tcc"
#include <cctype>

namespace LightSpeed {


extern LIGHTSPEED_EXPORT byte base64_encodeTable[65];
extern LIGHTSPEED_EXPORT byte base64_urlEncodeTable[65];
extern LIGHTSPEED_EXPORT byte base64_decodeTable[256];

void base64_initTables();

template<typename Bin, typename Char>
Base64EncoderT<Bin,Char>::Base64EncoderT(bool urlencode):buffer(0),cycle(0),eqchr(7),encodeTable(urlencode?base64_urlEncodeTable:base64_encodeTable) {}

template<typename Bin, typename Char>
bool Base64EncoderT<Bin,Char>::needItems() const {
	return cycle < 3;
}

template<typename Bin, typename Char>
void Base64EncoderT<Bin,Char>::input(const Bin& x) {
	natural q = (byte)x;
	buffer = buffer << 8 | q;
	cycle++;
}

template<typename Bin, typename Char>
bool Base64EncoderT<Bin,Char>::hasItems() const {
	return cycle >= 3 && cycle < 7;
}

template<typename Bin, typename Char>
Char Base64EncoderT<Bin,Char>::output() {
	byte q = (byte)(((buffer>>18 & 0x3F)));
	buffer<<=6;
	if (cycle >= eqchr) {
		q = '=';
		cycle++;
	} else {
		cycle++;
		if (cycle == 7)
			cycle = 0;

		q = encodeTable[q];
	}
	return (Char)q;
}

template<typename Bin, typename Char>
natural Base64EncoderT<Bin,Char>::calcToWrite(natural srcCount) const {
	return ((srcCount + 2) / 3) * 4;
}

template<typename Bin, typename Char>
natural Base64EncoderT<Bin,Char>::calcToRead(natural trgCount) const {
	return (trgCount / 4) * 3;
}

template<typename Bin, typename Char>
void Base64EncoderT<Bin,Char>::flush() {
	if (cycle == 0) {
		cycle = eqchr;
	} else {
		while (cycle < 3) {
			input(0);
			eqchr--;
		}
	}

}


template<typename Char, typename Bin>
bool Base64DecoderT<Char,Bin>::needItems() const {
	return cycle < 4;
}

template<typename Char, typename Bin>
void Base64DecoderT<Char,Bin>::input(const Char& xc) {
	char x = (char)xc;
	if (isspace(x)) return;
	if (x == '=' || x == '.' || x == '~') {
		eqchr--;
		buffer = buffer << 6;
		if (eqchr == 4) cycle = 0;
	} else {
		natural k = base64_decodeTable[(unsigned char)x];
		buffer = (buffer << 6) | (k);
		eqchr = 7;
	}
	cycle++;
}

template<typename Char, typename Bin>
bool Base64DecoderT<Char,Bin>::hasItems() const {
	return cycle >= 4 && cycle < eqchr;
}

template<typename Char, typename Bin>
Bin Base64DecoderT<Char,Bin>::output() {
	byte q = (buffer >> 16) & 0xFF;
	buffer <<= 8;
	cycle++;
	if (cycle == eqchr)
		cycle = 0;

	return (Bin)q;
}
template<typename Char, typename Bin>
natural Base64DecoderT<Char,Bin>::calcToWrite(natural srcCount) const {
	return ((srcCount + 2) / 3) * 4;
}

template<typename Char, typename Bin>
natural Base64DecoderT<Char,Bin>::calcToRead(natural trgCount) const {
	return (trgCount / 4) * 3;
}

template<typename Char, typename Bin>
bool Base64DecoderT<Char,Bin>::canAccept(const Char& x) const {
	return (base64_decodeTable[(unsigned char)x] != 0xFF);
}

template<typename Char, typename Bin>
Base64DecoderT<Char,Bin>::Base64DecoderT():buffer(0),cycle(0),eqchr(7) {
	base64_initTables();
}

template<typename Char, typename Bin>
void Base64DecoderT<Char,Bin>::flush() {
}



}

