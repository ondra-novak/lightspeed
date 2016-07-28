/*
 * lzw.tcc
 *
 *  Created on: Jul 26, 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_LZW_TCC_
#define LIGHTSPEED_UTILS_LZW_TCC_

#include "lzw.h"

#include "../base/exceptions/errorMessageException.h"

namespace LightSpeed {


template<typename Impl>
LZWCompressBase<Impl>::LZWCompressBase(natural maxBits)
	:nextChainCode(firstChainCode)
	,curChainCode(nullChainCode)
	,obits(0)
	,codeBits(initialBits)
	,maxChainCode((1 << ((maxBits>maxAllowedBits?maxAllowedBits:maxBits))) - 4) {
}

template<typename Impl>
const byte& LZWCompressBase<Impl>::getNext()  {

	const byte &out = outbuff.getNext();
	this->hasItems = outbuff.hasItems();
	this->needItems = !this->hasItems;
	return out;
}

template<typename Impl>
const byte& LZWCompressBase<Impl>::peek() const {

	return outbuff.peek();
}

template<typename Impl>
void LZWCompressBase<Impl>::writeClearCode(ChainCode code) {
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

template<typename Impl>
void LZWCompressBase<Impl>::increaseBits(ChainCode newCode) {
	//check, whether newCode is out of value able to encode by codeBits
	if (newCode == (1 << codeBits)) {
		//if does, extend codeBits
		codeBits++;
	}
}

template<typename Impl>
void LZWCompressBase<Impl>::write(const byte& b) {
	//calculate packed code
	//packed code is <curChainCode> | <byte>.
	natural packedCode = curChainCode << 8 | b;
	//packed code is used as lookup to the map.
	//if exists, value is next chain code
	typename DictTable::iterator itr = this->dictTable.find(packedCode);
	//found?
	if (itr == dictTable.end()) {
		//if not, chceck, whether curChainCodee is natural null
		if (curChainCode == nullChainCode) {
			//this can happen on first write. Make current byte as curChainCode
			curChainCode = b;
			//reset eolb is needed
			this->eolb = false;
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
				this->getImpl().onClearCode();
			}
			//use next byte as start of next chain
			curChainCode = b;
		}
	} else {
		//if chainCode found, use it as current for next round
		curChainCode = itr->second;
		itr->second.mark();
	}
}

template<typename Impl>
void LZWCompressBase<Impl>::flush() {
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
		outbuff.padzeroes();
	}
	this->needItems = true;
	this->eolb = true;
	dictTable.clear();
}

template<typename Impl>
void LZWCompressBase<Impl>::writeCode(ChainCode code) {
	outbuff.write(code,codeBits);
	this->hasItems = outbuff.hasItems();
	this->needItems = !this->hasItems;
}

template<typename Impl>
LZWDecompressBase<Impl>::LZWDecompressBase()
:prevCode(nullChainCode),accum(0),inbits(0),codeBits(initialBits)
{
}

template<typename Impl>
const byte& LZWDecompressBase<Impl>::getNext() {
	const byte &out = outbytes[outbytes.length()-1];
	outbytes.trunc(1);
	this->hasItems = !outbytes.empty();
	this->needItems = !this->hasItems;
	return out;
}

template<typename Impl>
const byte& LZWDecompressBase<Impl>::peek() const {
	const byte &out = outbytes[outbytes.length()-1];
	return out;
}

template<typename Impl>
void LZWDecompressBase<Impl>::clearDict() {
	codeBits = 9;
	dict.clear();
	this->hasItems = false;
	this->needItems = true;
	prevCode = ChainCode(-1);
}

template<typename Impl>
void LZWDecompressBase<Impl>::write(const byte& b) {
	ChainCode code;
	if (appendBits(b, code)) {
		if (code == clearCode) {
			this->getImpl().onClearCode();
		} else if (code == endData) {
			clearDict();
			inbits = 0;
			accum = 0;
			this->eolb = true;
		} else {
			ChainCode nextCode(dict.length()+firstChainCode);
			if (code < 256) {
				outbytes.clear();
				outbytes.add((byte)code);
				if (prevCode != nullChainCode) {
					dict.add(CodeInfo(code,prevCode));
				}
			} else if (code == nextCode) {
				if (prevCode == nullChainCode) {
					this->getImpl().onUnknownCode(code);
					return;
				}
				extractCode(prevCode,1);
				outbytes(0) = outbytes[outbytes.length()-1];
				CodeInfo nfo(outbytes[0],prevCode);
				//We need to mark this code, because it has been used in the same cycle as it was created.
				nfo.mark();
				dict.add(nfo);
			} else if (code  < nextCode) {
				extractCode(code,0);
				//this can happen after dictionary is partially cleared
				if (prevCode != nullChainCode) {
					CodeInfo nfo(outbytes[outbytes.length()-1],prevCode);
					dict.add(nfo);
				}
			} else {
				this->getImpl().onUnknownCode(code);
				return;
			}
			prevCode = code;
			if (nextCode+1 == (1 << codeBits)) {
				codeBits++;
			}
			this->hasItems = true;
			this->needItems = false;
			this->eolb = false;
		}
		this->needItems = !this->hasItems;
	}
}

template<typename Impl>
void LZWDecompressBase<Impl>::flush() {
}

template<typename Impl>
void LZWDecompressBase<Impl>::extractCode(natural code, natural reserve) {
	outbytes.clear();
	while (reserve--) outbytes.add(0);
	while (code >= firstChainCode) {
		const CodeInfo &cinfo = dict[code-firstChainCode];
		//mark code used
		cinfo.mark();
		outbytes.add(cinfo.outByte);
		code = cinfo.prevCode;
	}
	outbytes.add((byte)code);
}

template<typename Impl>
bool LZWDecompressBase<Impl>::appendBits(byte b, ChainCode &codeOut) {
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

template<typename Impl>
LZWCompressBase<Impl>::OutBuff::OutBuff()
	:wrpos(0),rdpos(0),wrbits(8),accum(0)
{
}

template<typename Impl>
const byte& LZWCompressBase<Impl>::OutBuff::getNext() {
	const byte &out = outbuff[rdpos];
	rdpos = (rdpos+1) & 0xF;
	return out;
}

template<typename Impl>
const byte& LZWCompressBase<Impl>::OutBuff::peek() {
	return outbuff[rdpos];
}

template<typename Impl>
void LZWCompressBase<Impl>::OutBuff::write(ChainCode code, byte bits) {
	while (bits >= wrbits) {
		bits-=wrbits;
		outbuff[wrpos] = accum | (code >> bits);
		wrpos = (wrpos + 1) & 0xF;
		code &= (1 << bits) - 1;
		wrbits = 8;
		accum = 0;
	}
	accum |= (code << (wrbits - bits));
	wrbits -= bits;
}

template<typename Impl>
bool LZWCompressBase<Impl>::OutBuff::hasItems() const {
	return rdpos != wrpos;
}

template<typename Impl>
void LZWCompressBase<Impl>::OutBuff::padzeroes() {
	outbuff[wrpos] = accum ;
	wrpos = (wrpos + 1) & 0xF;
	wrbits = 8;
	accum = 0;

}

template<typename Impl>
void LZWDecompressBase<Impl>::onUnknownCode(ChainCode) {
	throw ErrorMessageException(THISLOCATION, "LZW: Unexpected opcode - probably corrupted stram or new version" );
}


}



#endif /* LIGHTSPEED_UTILS_LZW_TCC_ */
