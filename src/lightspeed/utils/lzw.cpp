/*
 * lzw.cpp
 *
 *  Created on: 21. 7. 2016
 *      Author: ondra
 */

#include "lzw.h"

#include "../base/containers/autoArray.tcc"
namespace LightSpeed {


//const natural LZWCommonDefs::maxAllowedBits=15;

LZWCompress::LZWCompress(natural maxBits)
	:accumulator(0)
	,nextChainCode(firstChainCode)
	,curChainCode(nullChainCode)
	,obits(0)
	,codeBits(initialBits)
	,maxChainCode((1 << ((maxBits>maxAllowedBits?maxAllowedBits:maxBits)+1)) - 4)
	,partialClearCode(maxChainCode+3) {
}

const byte& LZWCompress::getNext()  {
	//8 bits will be written, decrease its count now
	obits-=8;
	//shitf left most bits to outbyte
	outbyte = (byte)(accumulator >> obits);
	//mask them out from accumulator
	accumulator &= ((1 << obits) - 1);
	//next state of hasitems depends on how much bits left in accumulator
	hasItems = obits >= 8;
	//we need items if there is at least maxBits bits available to store next chain code
	needItems = !hasItems;
	//return out byte
	return outbyte;
}

const byte& LZWCompress::peek() const {
	//just copy first byte
	outbyte = (byte)(accumulator >> (obits-8));
	//and output
	return outbyte;
}

void LZWCompress::writeClearCode(ChainCode code) {
	//write clear code
	writeCode(code);
	//set back to 9 bits
	codeBits = initialBits;
	//reset chain code counter
	nextChainCode = firstChainCode;
	//clear table
	dictTable.clear();
	curChainCode = nullChainCode;
}

void LZWCompress::increaseBits(ChainCode newCode) {
	//check, whether newCode is out of value able to encode by codeBits
	if (newCode == (1 << (codeBits + 1))) {
		//if does, extend codeBits
		codeBits++;
	}
}

void LZWCompress::write(const byte& b) {
	//calculate packed code
	//packed code is <curChainCode> | <byte>.
	natural packedCode = curChainCode << 8 | b;
	//packed code is used as lookup to the map.
	//if exists, value is next chain code
	DictTable::iterator itr = dictTable.find(packedCode);
	//found?
	if (itr == dictTable.end()) {
		//if not, chceck, whether curChainCodee is natural null
		if (curChainCode == nullChainCode) {
			//this can happen on first write. Make current byte as curChainCode
			curChainCode = b;
			//reset eolb is needed
			eolb = false;
		} else {
			//before we add new chain, write current chain to output
			writeCode(curChainCode);
			//introduce new chain code
			natural newCode = nextChainCode++;
			//insert chain code to the table with current packed code as key
			dictTable.insert(std::make_pair(packedCode,newCode));
			//check, whether newCode is out of value able to encode by codeBits
			increaseBits(newCode);
			if (newCode == maxChainCode) {
				//write clear code
				writeClearCode(clearCode);
			}
			//use next byte as start of next chain
			curChainCode = b;
		}
	} else {
		//if chainCode found, use it as current for next round
		curChainCode = itr->second;
	}
}

void LZWCompress::flush() {
	//flush fill finish current chain and write end-of-data
	//if there are chain
	if (curChainCode != nullChainCode) {
		//write it
		writeCode(curChainCode);
		//simulate increasing of the nextChainCode
		natural newCode = nextChainCode++;
		//
		increaseBits(newCode);
		writeClearCode(endData);
		while (obits & 0x7) {
			accumulator <<= 1;
			obits++;
		}
	}
	needItems = true;
	eolb = true;
	dictTable.clear();
}

void LZWCompress::writeCode(ChainCode code) {
	accumulator = (accumulator << codeBits) | code;
	obits+=codeBits;
	hasItems = obits >= 8;
	needItems = !hasItems;
}

LZWDecompress::LZWDecompress()
:accum(0),prevCode(nullChainCode),inbits(0),codeBits(initialBits)
{
}

const byte& LZWDecompress::getNext() {
	const byte &out = outbytes[outbytes.length()-1];
	outbytes.trunc(1);
	hasItems = !outbytes.empty();
	needItems = !hasItems;
	return out;
}

const byte& LZWDecompress::peek() const {
	const byte &out = outbytes[outbytes.length()-1];
	return out;
}

void LZWDecompress::clearDict() {
	codeBits = 9;
	dict.clear();
	hasItems = false;
	needItems = true;
	prevCode = ChainCode(-1);
}

void LZWDecompress::write(const byte& b) {
	ChainCode code;
	if (appendBits(b, code)) {
		if (code == clearCode || code == endData) {
			clearDict();
			if (code == endData) {
				inbits = 0;
				accum = 0;
				eolb = true;
			}
		} else {
			ChainCode nextCode(dict.length()+firstChainCode);
			if (code < 256) {
				outbytes.clear();
				outbytes.add((byte)code);
			} else if (code == nextCode) {
				extractCode(prevCode,1);
				outbytes(0) = outbytes[outbytes.length()-1];
			} else {
				extractCode(code,0);
			}
			if (prevCode != nullChainCode) {
				dict.add(CodeInfo(outbytes[0],prevCode));
			}
			prevCode = code;
			if (nextCode+1 == (1 << (codeBits+1))) {
				codeBits++;
			}
			hasItems = true;
			needItems = false;
			eolb = false;
		}
		needItems = !hasItems;
	}
}

void LZWDecompress::flush() {
}

void LZWDecompress::extractCode(natural code, natural reserve) {
	outbytes.clear();
	while (reserve--) outbytes.add(0);
	while (code >= firstChainCode) {
		const CodeInfo &cinfo = dict[code-firstChainCode];
		outbytes.add(cinfo.outByte);
		code = cinfo.prevCode;
	}
	outbytes.add((byte)code);
}

bool LZWDecompress::appendBits(byte b, ChainCode &codeOut) {
	accum = (accum << 8) | b;
	inbits+=8;
	if (inbits >=codeBits) {
		inbits -= codeBits;
		codeOut = ChainCode(accum >> inbits);
		accum &= (1<<inbits)-1;
		return true;
	} else {
		return false;
	}
}
}
