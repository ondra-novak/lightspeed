/*
 * base64.cpp
 *
 *  Created on: Oct 15, 2012
 *      Author: ondra
 */




#include "base16.h"

#include <ctype.h>
namespace LightSpeed {


byte base16uppers_table[17] = "0123456789ABCDEF";
byte base16lowers_table[17] = "0123456789abcdef";

const ConstStrA ByteToBase16Convert::hexUpperCase((char *)base16uppers_table,16);
const ConstStrA ByteToBase16Convert::hexLowerCase((char *)base16lowers_table,16);
const ConstStrA Base16ToByteConvert::hexUpperCase((char *)base16uppers_table,16);
const ConstStrA Base16ToByteConvert::hexLowerCase((char *)base16lowers_table,16);


ByteToBase16Convert::ByteToBase16Convert()
:table(hexUpperCase),readpos(0)
{

}
ByteToBase16Convert::ByteToBase16Convert(ConstStrA table)
:table(table),readpos(0)
{

}

const char &ByteToBase16Convert::getNext() {
	const char &out = outchars[readpos];
	readpos = (readpos+1) & 1;
	needItems = readpos == 0;
	hasItems = !needItems;
	return out;
}
const char &ByteToBase16Convert::peek() const {
	return outchars[readpos];
}
void ByteToBase16Convert::write(const byte &item) {

	outchars[0] = table[item >> 4];
	outchars[1] = table[item & 0xF];
	needItems = false;
	hasItems = true;
	readpos = 0;
}

void ByteToBase16Convert::flush() {
}

Base16ToByteConvert::Base16ToByteConvert()
	:wrpos(0)
{
	initTable(hexUpperCase, false);
}

Base16ToByteConvert::Base16ToByteConvert(ConstStrA table, bool caseSensitive)
	:wrpos(0)
	 {
	initTable(table,caseSensitive);
}

const byte &Base16ToByteConvert::getNext() {
	hasItems = false;
	needItems = true;
	return outByte;
}

const byte &Base16ToByteConvert::peek() const {
	return outByte;
}
void Base16ToByteConvert::write(const char &item) {
	if (wrpos) {
		outByte |= table[(int)((byte)item)];
		wrpos = 0;
		hasItems = true;
		needItems = false;
	} else {
		outByte = table[(int)((byte)item)]<<4;
		wrpos = 1;
		hasItems = false;
		needItems = true;
	}
}
void Base16ToByteConvert::flush() {
	if (wrpos == 1) write(0);
}
void Base16ToByteConvert::initTable(ConstStrA t, bool caseSensitive = false) {
	//for performance reason, do not fill other items, it expects valid format if base64

	if (caseSensitive) {
		for (natural i = 0; i < t.length(); i++) {
			table[(unsigned int)(toupper(t[i]))] = (byte)(i & 0xF);
			table[(unsigned int)(tolower(t[i]))] = (byte)(i & 0xF);
		}

	} else {
		for (natural i = 0; i < t.length(); i++)
			table[(unsigned int)(t[i])] = (byte)(i & 0xF);

	}

}



}

