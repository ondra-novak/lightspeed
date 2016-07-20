/*
 * lzw.cpp
 *
 *  Created on: 21. 7. 2016
 *      Author: ondra
 */

#include "lzw.h"

namespace LightSpeed {


LZWCompress::LZWCompress()
	:accumulator(0)
	,nextChainCode(firstChainCode)
	,curChainCode(naturalNull)
	,obits(0)
	,codeBits(initialBits) {
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
	needItems = obits <= sizeof(natural)*8-maxBits;
	//return out byte
	return outbyte;
}

const byte& LZWCompress::peek() const {
	//just copy first byte
	outbyte = (byte)(accumulator >> (obits-8));
	//and output
	return outbyte;
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
		if (curChainCode == naturalNull) {
			//this can happen on first write. Make current byte as curChainCode
			curChainCode = packedCode;
			//reset eolb is needed
			eolb = false;
		} else {
			//before we add new chain, write current chain to output
			writeCode(curChainCode);
			//introduce new chain code
			natural newCode = nextChainCode+1;
			//insert chain code to the table with current packed code as key
			dictTable.insert(std::make_pair(packedCode,newCode));
			//check, whether newCode is out of value able to encode by codeBits
			if (newCode == (1 << (codeBits+1))) {
				//if does, extend codeBits
				codeBits++;
				//if maxBits reached
				if (codeBits>maxBits) {
					//write clear code
					writeCode(clearCode);
					//set back to 9 bits
					codeBits=9;
					//reset chain code counter
					nextChainCode = firstChainCode;
					//clear table
					dictTable.clear();
				}
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
	if (curChainCode != naturalNull) {
		//write it
		writeCode(curChainCode);
		//simulate increasing of the nextChainCode
		natural newCode = nextChainCode+1;
		//checking increasing bit count
		if (newCode == (1 << (codeBits+1)))
			//extend if necesery
			codeBits++;
		//write endOfData mark
		writeCode(endData);
		//reset back to factory settings
		codeBits=9;
		nextChainCode = firstChainCode;
		hasItems = true;
		curChainCode = naturalNull;
	}
	needItems = true;
	eolb = true;
	dictTable.clear();
}

void LZWCompress::writeCode(natural code) {
	accumulator = (accumulator << codeBits) | code;
	hasItems = obits >= 8;
	needItems = obits <= sizeof(natural)*8-(maxBits+1);
}

}
