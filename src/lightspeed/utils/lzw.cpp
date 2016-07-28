/*
 * lzw.cpp
 *
 *  Created on: 21. 7. 2016
 *      Author: ondra
 */

#include "lzw.tcc"

#include "../base/containers/autoArray.tcc"
#include "../base/exceptions/errorMessageException.h"
namespace LightSpeed {


LZWpCompress::LZWpCompress(natural maxBits)
:LZWCompressBase<LZWpCompress>(maxBits),optimizeStart((1<<maxBits)-5),optimizeCode((1<<maxBits)-1)
{
}

void LZWpCompress::onClearCode() {
	writeCode(clearCode);
	optimizeDictionary();
}

void LZWpCompress::optimizeDictionary() {
	std::map<ChainCode, ChainCode> codeMap;
	std::map<ChainCode, natural> revMap;

	//generate list of new codes and assign it to choosen chains
	for(DictTable::iterator iter = dictTable.begin(); iter != dictTable.end() ;++iter) {
		if (iter->second.marked()) {
			revMap.insert(std::make_pair(iter->second.code,iter->first));
		}
	}

	nextChainCode = firstChainCode;
	codeBits = 9;
	dictTable.clear();


	for(std::map<ChainCode, natural>::iterator iter = revMap.begin(); iter != revMap.end();++iter) {
		byte bout = (byte)(iter->second & 0xFF);
		ChainCode prevCode = (ChainCode)(iter->second >> 8);
		ChainCode newCode = nextChainCode++;
		increaseBits(newCode);

		if (prevCode >= firstChainCode) {
			std::map<ChainCode, ChainCode>::iterator newPrevCode = codeMap.find(prevCode);
			if (newPrevCode == codeMap.end()) {
				throw ErrorMessageException(THISLOCATION, "LZW:Corrupted dictionary");
			}
			prevCode = newPrevCode->second;
		}

		natural packedCode = prevCode << 8 | bout;
		dictTable.insert(std::make_pair(packedCode,newCode));
		codeMap.insert(std::make_pair(iter->first, newCode));
	}

/*

	AutoArray<natural> codeRevMap;
	AutoArray<natural> codeRevMap2;
	DictTable newTable;
	DictTable newTable2;

	ChainCode stopCode = nextChainCode - 50;
	codeMap.resize(nextChainCode,0);
	codeRevMap.resize(nextChainCode,naturalNull);
	codeRevMap2.reserve(codeRevMap.length());

	//generate list of new codes and assign it to choosen chains
	for(DictTable::iterator iter = dictTable.begin(); iter != dictTable.end() && nextChainCode < stopCode ;++iter) {
		if (iter->second.marked()) {
			codeRevMap(iter->second.code-firstChainCode) = iter->first;
		}
	}

	for (natural i = 0; i < codeRevMap.length(); i++) {
		if (codeRevMap[i] != naturalNull) {
			ChainCode newCode = nextChainCode++;
			increaseBits(newCode);
			codeMap(i) = codeRevMap2.length()+firstChainCode;
			codeRevMap2.add(codeRevMap[i]);
		}
	}

	dictTable.clear();
	for (natural i = 0; i < codeRevMap2.length(); i++) {
		byte bout = (byte)(codeRevMap2[i] & 0xFF);
		ChainCode prevCode = (ChainCode)(codeRevMap2[i] >> 8);
		if (prevCode >= firstChainCode) {
			ChainCode newPrevCode = codeMap[prevCode-firstChainCode];
			if (newPrevCode == 0) {
				throw ErrorMessageException(THISLOCATION, "LZW:Corrupted dictionary");
			}
			prevCode = newPrevCode;
		}
		natural packedCode = prevCode << 8 | bout;
		dictTable.insert(std::make_pair(packedCode,i+firstChainCode));
	}*/
}



void LZWpDecompress::onClearCode() {
	optimizeDictionary();
}

void LZWpDecompress::optimizeDictionary() {
	typedef typename LZWCommonDefs::ChainCode ChainCode;
	typedef typename LZWDecompressBase<LZWpDecompress>::CodeInfo CodeInfo;


	AutoArray<ChainCode> optTable;
	AutoArray<CodeInfo> newTable;

	this->codeBits = 9;
	ChainCode curCode(this->dict.length()+firstChainCode);
	optTable.resize(curCode,0);
	ChainCode nextCode(firstChainCode);


	for(natural i = 0; i < dict.length();i++) {
		const CodeInfo &nfo = dict[i];
		if (nfo.marked) {
			byte bout = nfo.outByte;
			ChainCode prevCode = nfo.prevCode;
			if (prevCode >= firstChainCode) {
				prevCode = optTable[prevCode - firstChainCode];
				if (prevCode == 0) {
					throw ErrorMessageException(THISLOCATION, "LZW:Corrupted dictionary");
				}
			}
			optTable(i) = nextCode;
			newTable.add(CodeInfo(bout, prevCode));
			if (nextCode+1 == (1 << codeBits)) {
				codeBits++;
			}
			nextCode++;
		}
	}
	dict.swap(newTable);

	this->hasItems = false;
	this->needItems = true;
	prevCode = ChainCode(-1);


}

void LZWCompress::onClearCode() {
	writeClearCode(clearCode);
}

void LZWDecompress::onClearCode() {
	clearDict();

}

template class LZWCompressBase<LZWCompress>;
template class LZWCompressBase<LZWpCompress>;
template class LZWDecompressBase<LZWDecompress>;
template class LZWDecompressBase<LZWpDecompress>;

}
