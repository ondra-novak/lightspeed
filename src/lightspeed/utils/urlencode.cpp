/*
 * urlencode.cpp
 *
 *  Created on: 23.4.2013
 *      Author: ondra
 */

#include "urlencode.h"

#include "base16.h"
namespace LightSpeed {


const char& UrlEncodeConvert::getNext() {
	const char &out = buff[readpos];
	readpos = readpos + 1;
	if (readpos == maxread) {
		readpos = 0;
		hasItems = false;
		needItems = true;
	}

	return out;
}

const char& UrlEncodeConvert::peek() const {
	const char &out = buff[readpos];
	return out;
}

void UrlEncodeConvert::write(const char& x) {
	if ((x >='A' && x <='Z') || (x >= 'a' && x <= 'z')
				|| (x >='0' && x <='9') || x == '_' || x == '-') {
			buff[0] = x;
			maxread=1;
	} else {
		buff[0] = '%';
		ByteToBase16Convert conv;
		conv.write(x);
		buff[1] = conv.getNext();
		buff[2] = conv.getNext();
		maxread=3;
	}
	hasItems = true;
	needItems = false;
}


UrlDecodeConvert::UrlDecodeConvert():wrpos(0) {
}

const char& UrlDecodeConvert::getNext() {
	needItems = true;
	hasItems = false;
	return buff[0];
}

const char& UrlDecodeConvert::peek() const {
	return buff[0];
}

void UrlDecodeConvert::write(const char& x) {
	if (wrpos) {
		buff[wrpos] = x;
		wrpos++;
		if (wrpos != 3) return;
		Base16ToByteConvert conv;
		conv.write(buff[1]);
		conv.write(buff[2]);
		buff[0] = (char)conv.getNext();
		wrpos = 0;
	} else if (x == '+') {
		buff[0] = 32;
	} else {
		buff[0] = x;
		if (x == '%') {
			wrpos = 1;
			return;
		}
	}
	needItems = false;
	hasItems = true;
}


}
