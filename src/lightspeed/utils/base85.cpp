/*
 * base64.cpp
 *
 *  Created on: Oct 15, 2012
 *      Author: ondra
 */




#include "base85.h"

namespace LightSpeed {

//  Maps base 256 to base 85
static char encoder [85 + 1] = {
    "0123456789"
    "abcdefghij"
    "klmnopqrst"
    "uvwxyzABCD"
    "EFGHIJKLMN"
    "OPQRSTUVWX"
    "YZ.-:+=^!/"
    "*?&<>()[]{"
    "}@%$#"
};

//  Maps base 85 to base 256
//  We chop off lower 32 and higher 128 ranges
static byte decoder [96] = {
    0x00, 0x44, 0x00, 0x54, 0x53, 0x52, 0x48, 0x00,
    0x4B, 0x4C, 0x46, 0x41, 0x00, 0x3F, 0x3E, 0x45,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x40, 0x00, 0x49, 0x42, 0x4A, 0x47,
    0x51, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A,
    0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32,
    0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
    0x3B, 0x3C, 0x3D, 0x4D, 0x00, 0x4E, 0x43, 0x00,
    0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x4F, 0x00, 0x50, 0x00, 0x00
};

static Bin::natural32 multiplies[5] = {1,85,85*85,85*85*85,85*85*85*85};

ByteToBase85Convert::ByteToBase85Convert()
:accum(0),wrpos(0),readpos(0),maxreadpos(0)
{
}

void ByteToBase85Convert::reset() {
	readpos = 0;
	wrpos = 0;
	needItems = true;
	hasItems = false;
	accum = 0;
}

const char &ByteToBase85Convert::getNext() {
	const char &out = outchars[readpos];
	readpos = readpos+1;
	if (readpos == maxreadpos) {
		reset();
	}
	return out;
}
const char &ByteToBase85Convert::peek() const {
	return outchars[readpos];
}
void ByteToBase85Convert::write(const byte &item) {



	//if eolb is active - reset state of convertor when new item is written
	if (eolb) {
		eolb = false;
		reset();
	}

	accum = (accum << 8) | item;
	wrpos++;
	if (wrpos > 3) {
		int pos = 0;

		for (natural i = 0; i < 5; i++) {
			int x = accum/multiplies[4-i] % 85;
			outchars[pos++] = encoder[x];
		}
		maxreadpos = pos;
		needItems = false;
		hasItems = true;
	}

}

void ByteToBase85Convert::flush() {
	if (this->eolb) return;

	if (wrpos) {
		natural remain = 4 - wrpos;

		for (natural i = 0; i < remain; i++) write(0);
		maxreadpos -= remain;
	}
	this->eolb = true;
	this->needItems = false;
	this->hasItems = true;
}

Base85ToByteConvert::Base85ToByteConvert()
	:accum(0),readpos(0),wrpos(0)
{

}

void Base85ToByteConvert::reset() {
	readpos = 0;
	wrpos = 0;
	needItems = true;
	hasItems = false;
	accum = 0;
}

const byte &Base85ToByteConvert::getNext() {
	const byte &out = outBytes[readpos];
	readpos = readpos+1;
	if (readpos == maxreadpos) {
		reset();
	}
	return out;
}

const byte &Base85ToByteConvert::peek() const {
	return outBytes[readpos];
}
void Base85ToByteConvert::write(const char &item) {

	if ((signed char) item < 33) return;
	//if eolb is active - reset state of convertor when new item is written
	if (eolb) {
		eolb = false;
		reset();
	}
	accum = accum * 85 + decoder [(byte) item - 32];
	wrpos++;
	if (wrpos == 5) {
		for (natural i = 0; i < 4; i++) {
			outBytes[i] = (byte)(accum >> ((3-i)*8));
		}
		maxreadpos=4;
		needItems = false;
		hasItems = true;
	}
}
void Base85ToByteConvert::flush() {
	if (wrpos) {
		natural remain = 5-wrpos;
		for (natural i = 0; i < remain; i++)
			write(encoder[84]);
		maxreadpos-=remain;
	}
	this->eolb = true;
}

}

