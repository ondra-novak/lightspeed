/*
 * base64.cpp
 *
 *  Created on: Oct 15, 2012
 *      Author: ondra
 */




#include "base64.tcc"

namespace LightSpeed {


byte base64_encodeTable[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
byte base64_urlEncodeTable[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
byte base64_decodeTable[256] = "";

void base64_initTables() {
	if (base64_decodeTable[0] == 0) {
		for (byte i = 0; i < 255; i++) base64_decodeTable[i] = 0xFF;
		for (byte i = 0; i < 64; i++)
			base64_decodeTable[base64_encodeTable[i]] = (byte)((i));

		base64_decodeTable[(unsigned int)'-'] = 62;
	        base64_decodeTable[(unsigned int)'_'] = 63;
	}
}



template class Base64EncoderT<char,char>;
template class Base64EncoderT<byte,char>;
template class Base64EncoderT<byte,byte>;

template class Base64DecoderT<char,byte>;
template class Base64DecoderT<char,char>;
template class Base64DecoderT<byte,byte>;

}

