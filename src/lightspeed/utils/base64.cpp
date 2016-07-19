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

const ConstStrA ByteToBase64Convert::standardTable((char *)base64_encodeTable,64);
const ConstStrA ByteToBase64Convert::forUrlTable((char *)base64_urlEncodeTable,64);
const ConstStrA Base64ToByteConvert::standardTable((char *)base64_encodeTable,64);
const ConstStrA Base64ToByteConvert::forUrlTable((char *)base64_urlEncodeTable,64);

ByteToBase64Convert::ByteToBase64Convert()
:table(standardTable),padChar(standardPadding),wrbyte(0),readpos(0)
{

}
ByteToBase64Convert::ByteToBase64Convert(ConstStrA table, char paddingChar)
:table(table),padChar(paddingChar),wrbyte(0),readpos(0)
{

}


const char &ByteToBase64Convert::getNext() {
	const char &out = outchars[readpos];
	readpos = readpos+1;
	if (readpos == 4) {
		readpos = 0;
		wrbyte = 0;
		needItems = true;
		hasItems = false;
	} else {
		hasItems = readpos < wrbyte;
	}
	return out;
}
const char &ByteToBase64Convert::peek() const {
	return outchars[readpos];
}
void ByteToBase64Convert::write(const byte &item) {

	//if eolb is active - reset state of convertor when new item is written
	if (this->eolb) {
		this->eolb = false;
		wrbyte = 0;
		readpos = 0;
	}

	switch (wrbyte) {
	case 0:
		//encode first 6 bits to character
		outchars[0] = table[item>>2];
		//store partial of the result - we will need it in next cycle
		outchars[1] = (item & 0x3) << 4;
		//increase write counter
		++wrbyte;
		break;
	case 1:
		//retrieve stored result and combine it with four bits from second byte
		outchars[1] = table[outchars[1] | (item >> 4)];
		//store remaining four bits - we will need it in next cycle
		outchars[2] = (item & 0xF) << 2;
		//increase write counter
		++wrbyte;
		break;
	case 2:
		//retrieve stored result and combine it with six bits from the third byte
		outchars[2] = table[outchars[2] | (item >> 6)];
		//also encode remaining six bits
		outchars[3] =  table[item & 0x3F];
		//increase write counter twice - it unlocks last character for reading
		++wrbyte;
		++wrbyte;
		//lock need items - because now reader must finish reading of generated four characters
		needItems = false;

		break;
		//cannot write now, throw exception
	default: throwIteratorNoMoreItems(THISLOCATION,typeid(*this));
		     break;
	}
	hasItems = true;
}

void ByteToBase64Convert::flush() {
	//flush should finish current base64 sequence
	//it is called when writting and source iterator is closed
	//first check, whether flush has been called, we will use eolb flag
	//if did, do nothing now
	if (this->eolb) return;
	//set eolb, because no more data are expected
	this->eolb = true;
	//close needItems, no more items are expected
	this->needItems = false;
	//depend on current state
	switch (wrbyte) {
		case 0:
			//no bytes written, do nothing, hasItems will be false
			break;
		case 1:
			//one byte written
			//encode second char from stored remaining bits and padded by zeroes
			outchars[1] = table[outchars[1]];
			if (padChar) {
				//write pad char
				outchars[2] = padChar;
				//write pad char
				outchars[3] = padChar;
				//adjust wrbyte
				wrbyte=4;
			} else {
				wrbyte=2;
			}
			break;
		case 2:
			//two bytes written
			//encode third char from stored remaining bots and padded by zeroes
			outchars[2] = table[outchars[2]];
			if (padChar) {
				//write pad char
				outchars[3] = padChar;
				//adjust wrbytes
				wrbyte=4;
			} else {
				wrbyte=3;
			}
			break;
		default:
			//three bytes writen, nothing need to be done
			break;
	}
	//unlock hasItems only if there are chars to read.
	hasItems = readpos < wrbyte;
}

Base64ToByteConvert::Base64ToByteConvert()
	:padChar(standardPadding),readpos(0),wrpos(0) {
	initTable(standardTable);
	table[(unsigned int)'-'] = 62;
    table[(unsigned int)'_'] = 63;

}

Base64ToByteConvert::Base64ToByteConvert(ConstStrA table, char paddingChar)
	:padChar(paddingChar),readpos(0),wrpos(0) {
	initTable(table);
}

const byte &Base64ToByteConvert::getNext() {
	const byte &out = outBytes[readpos];
	readpos = readpos+1;
	if (readpos == 3) {
		readpos = 0;
		wrpos = 0;
		needItems = true;
		hasItems = false;
	} else {
		hasItems = readpos+1 < wrpos;
	}
	return out;
}

const byte &Base64ToByteConvert::peek() const {
	return outBytes[readpos];
}
void Base64ToByteConvert::write(const char &item) {
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
			case 0: outBytes[0] = table[x] << 2;
					++wrpos;
					break;
			case 1: outBytes[0] |= table[x] >> 4;
					outBytes[1] = table[x] << 4;
					++wrpos;
					break;
			case 2: outBytes[1] |= table[x] >> 2;
					outBytes[2] = table[x] << 6;
					++wrpos;
					break;
			case 3: outBytes[2] |= table[x];
					++wrpos;
					break;
			default:throwIteratorNoMoreItems(THISLOCATION,typeid(*this));
					 break;
		}
		hasItems = readpos+1 < wrpos;
	}
}
void Base64ToByteConvert::flush() {
	this->eolb = true;
}
void Base64ToByteConvert::initTable(ConstStrA t) {
	//for performance reason, do not fill other items, it expects valid format if base64
	for (natural i = 0; i < t.length(); i++)
		table[(unsigned int)(t[i])] = (byte)(i & 63);

}





void base64_initTables() {
	if (base64_decodeTable[0] == 0) {
		for (natural i = 0; i < 256; i++) base64_decodeTable[i] = 0xFF;
		for (natural i = 0; i < 64; i++)
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

