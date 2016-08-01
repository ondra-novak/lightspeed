/*
 * base64.cpp
 *
 *  Created on: Oct 15, 2012
 *      Author: ondra
 */



#include "base32.h"

#include <ctype.h>

namespace LightSpeed {


byte base32_RFC4648[33] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

const ConstStrA ByteToBase32Convert::standardTable((char *)base32_RFC4648,32);
const ConstStrA Base32ToByteConvert::standardTable((char *)base32_RFC4648,32);


ByteToBase32Convert::ByteToBase32Convert()
:table(standardTable),padChar(standardPadding),wrbyte(0),readpos(0)
{

}
ByteToBase32Convert::ByteToBase32Convert(ConstStrA table, char paddingChar)
:table(table),padChar(paddingChar),wrbyte(0),readpos(0)
{

}


const char &ByteToBase32Convert::getNext() {
	const char &out = outchars[readpos];
	readpos = readpos+1;
	if (readpos == 8) {
		readpos = 0;
		wrbyte = 0;
		needItems = true;
		hasItems = false;
	} else {
		hasItems = readpos < wrbyte*8/5;
	}
	return out;
}
const char &ByteToBase32Convert::peek() const {
	return outchars[readpos];
}
void ByteToBase32Convert::write(const byte &item) {

	//if eolb is active - reset state of convertor when new item is written
	if (this->eolb) {
		this->eolb = false;
		wrbyte = 0;
		readpos = 0;
	}

	switch (wrbyte) {
	case 0:
		outchars[0] = table[item>>3]; //000XXXXX
		outchars[1] = (item & 0x7) << 2; //000XXX00
		++wrbyte;
		break;
	case 1:
		outchars[1] = table[outchars[1] | (item >> 6)]; //000XXXYY
		outchars[2] = table[(item>>1) & 0x1F]; //000YYYYY
		outchars[3] = (item & 0x1) << 4; //000Y0000
		++wrbyte;
		break;
	case 2:
		outchars[3] = table[outchars[3] | (item >> 4)]; //000YZZZZ
		outchars[4] =  (item & 0xF) << 1; //000ZZZZ0
		++wrbyte;

		break;
	case 3:
		outchars[4] = table[outchars[4] | (item >> 7)]; //000ZZZZA
		outchars[5] =  table[(item>>2) & 0x1F] ; //000AAAAA
		outchars[6] =  (item & 0x3) << 3 ; //000AA000
		++wrbyte;
		break;
	case 4:
		outchars[6] = table[outchars[6] | (item >> 5)]; //000AABBB
		outchars[7] =  table[item & 0x1F]; //000BBBBB
		++wrbyte;

		needItems = false;
		break;
		//cannot write now, throw exception
	default: throwIteratorNoMoreItems(THISLOCATION,typeid(*this));
		     break;
	}
	hasItems = true;
}

void ByteToBase32Convert::flush() {
	//flush should finish current base64 sequence
	//it is called when writting and source iterator is closed
	//first check, whether flush has been called, we will use eolb flag
	//if did, do nothing now
	if (this->eolb) return;
	//set eolb, because no more data are expected
	this->eolb = true;
	//close needItems, no more items are expected
	this->needItems = false;

	if (wrbyte > 0 && wrbyte < 5) {
		int idx = wrbyte * 8 / 5;
		outchars[idx] = table[outchars[idx]];
		if (padChar) {
			idx++;
			while (idx < 8) {
				outchars[idx] = padChar;
				idx++;
			}
			wrbyte = 5;
		}
	}

	//unlock hasItems only if there are chars to read.
	hasItems = readpos < wrbyte*8/5;
}

Base32ToByteConvert::Base32ToByteConvert()
	:padChar(standardPadding),readpos(0),wrpos(0) {
	initTable(standardTable,false);
}

Base32ToByteConvert::Base32ToByteConvert(ConstStrA table, char paddingChar, bool caseSensitive)
	:padChar(paddingChar),readpos(0),wrpos(0) {
	initTable(table,caseSensitive);
}

const byte &Base32ToByteConvert::getNext() {
	const byte &out = outBytes[readpos];
	readpos = readpos+1;
	if (readpos == 5) {
		readpos = 0;
		wrpos = 0;
		needItems = true;
		hasItems = false;
	} else {
		hasItems = readpos < wrpos * 5/8;
	}
	return out;
}

const byte &Base32ToByteConvert::peek() const {
	return outBytes[readpos];
}
void Base32ToByteConvert::write(const char &item) {
	if (item == padChar) {
		//toto je spravne - pouze cely bajt muze byt precten a znaky navic jsou jen do padding
		this->eolb = true;
	} else {
		//if eolb is active - reset state of convertor when new item is written
		if (this->eolb) {
			this->eolb = false;
			wrpos = 0;
			readpos = 0;
		}
		int x = (byte)item;
		switch (wrpos) {
			case 0: outBytes[0] = table[x] << 3;  //AAAAA000
					++wrpos;
					break;
			case 1: outBytes[0] |= table[x] >> 2; //AAAAABBB
					outBytes[1] = table[x] << 6;  //BB000000
					++wrpos;
					break;
			case 2: outBytes[1] |= table[x] << 1; //BBCCCCC0
					++wrpos;
					break;
			case 3: outBytes[1] |= table[x] >> 4; //BBCCCCCD
					outBytes[2] = table[x] << 4;  //DDDD0000
					++wrpos;
					break;
			case 4: outBytes[2] |= table[x] >> 1; //DDDDEEEE
					outBytes[3] = table[x] << 7;  //E0000000
					++wrpos;
					break;
			case 5: outBytes[3] |= table[x] << 2; //EFFFFF00
					++wrpos;
					break;
			case 6: outBytes[3] |= table[x] >> 3; //EFFFFFGG
					outBytes[4] = table[x] << 5;  //GGG00000
					++wrpos;
					break;
			case 7: outBytes[4] |= table[x];      //GGGHHHHH
					++wrpos;
					break;
			default:throwIteratorNoMoreItems(THISLOCATION,typeid(*this));
					 break;
		}
		hasItems = readpos < wrpos * 5/8;
	}
}
void Base32ToByteConvert::flush() {
	this->eolb = true;
}
void Base32ToByteConvert::initTable(ConstStrA t, bool caseSensitive = false) {
	//for performance reason, do not fill other items, it expects valid format if base64

	if (caseSensitive) {
		for (natural i = 0; i < t.length(); i++) {
			table[(unsigned int)(toupper(t[i]))] = (byte)(i & 31);
			table[(unsigned int)(tolower(t[i]))] = (byte)(i & 31);
		}

	} else {
		for (natural i = 0; i < t.length(); i++)
			table[(unsigned int)(t[i])] = (byte)(i & 31);

	}

}



}

