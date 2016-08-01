/*
 * lzw.cpp
 *
 *  Created on: 21. 7. 2016
 *      Author: ondra
 */

#include "lzw.tcc"

#include <map>
#include "../base/containers/autoArray.tcc"
#include "../base/exceptions/errorMessageException.h"
namespace LightSpeed {


LZWpCompress::LZWpCompress(natural maxBits)
:LZWCompressBase<LZWpCompress>(maxBits)
{
}

void LZWpCompress::onClearCode() {
	writeCode(clearCode);
	optimizeDictionary();
}

void LZWpCompress::optimizeDictionary() {
	std::map<ChainCode, ChainCode> codeMap;
	std::map<ChainCode, natural> revMap;

	natural stopCode = nextChainCode - (lzwpDictSpace+1);

	//generate list of new codes and assign it to choosen chains
	for(Dictionary::Iterator iter = dictTable.getFwIter(); iter.hasItems();) {
		const Dictionary::Iterator::ItemT &itm = iter.getNext();
		if (itm.second->marked() && itm.second->code < stopCode) {
			revMap.insert(std::make_pair(itm.second->code,itm.first << 8 | itm.second->ifbyte));
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

		dictTable.addCode(prevCode,bout,newCode);
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
	natural maxDict = this->dict.length()-lzwpDictSpace;
	optTable.resize(curCode,0);
	ChainCode nextCode(firstChainCode);


	for(natural i = 0; i < maxDict;i++) {
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

const LZWCommonDefs::ChainCode* LZWCommonDefs::DictItem::findCode(byte b) const {
	if (b == ifbyte) return &code;
	DictItem *p = nextItem;
	while (p != 0) {
		if (p->ifbyte == b) return &p->code;
		p = p->nextItem;
	}
	return 0;
}

const LZWCommonDefs::ChainCode* LZWCommonDefs::DictItem::findCodeAndMark(byte b) {
	if (b == ifbyte) {
		track = true;
		return &code;
	}
	DictItem *p = nextItem;
	while (p != 0) {
		if (p->ifbyte == b) {
			p->track = true;
			return &p->code;
		}
		p = p->nextItem;
	}
	return 0;
}

const LZWCommonDefs::ChainCode* LZWCommonDefs::Dictionary::findCode(ChainCode prevCode,byte b) const {
	if (prevCode < codeList.length() && codeList[prevCode]!=null) {
		return codeList[prevCode]->findCode(b);
	} else {
		return 0;
	}
}

const LZWCommonDefs::ChainCode* LZWCommonDefs::Dictionary::findCodeAndMark(ChainCode prevCode,byte b)  {
	if (prevCode < codeList.length() && codeList[prevCode]!=null) {
		return codeList.mutableAt(prevCode)->findCodeAndMark(b);
	} else {
		return 0;
	}
}

void LZWCommonDefs::Dictionary::addCode(ChainCode prevCode, byte b, ChainCode nextCode) {
	if (prevCode >= codeList.length()) codeList.resize(prevCode+1);
	PItem itm = new(allocator) DictItem(b,nextCode,codeList[prevCode]);
	codeList.set(prevCode, itm);
}

void LZWCommonDefs::Dictionary::clear() {
	codeList.clear();
}

LZWCommonDefs::Dictionary::Iterator LZWCommonDefs::Dictionary::getFwIter() const {
	return Iterator(codeList);

}

LZWCommonDefs::Dictionary::Iterator::Iterator(ConstStringT<PItem> items)
	:items(items)
{
	nextItem.first = naturalNull;
	nextItem.second = 0;
	fetch();
}

bool LZWCommonDefs::Dictionary::Iterator::hasItems() const {
	return (nextItem.first < items.length());
}

const LZWCommonDefs::Dictionary::Iterator::ItemT& LZWCommonDefs::Dictionary::Iterator::peek() const {
	outItem = nextItem;
	return outItem;
}

const LZWCommonDefs::Dictionary::Iterator::ItemT& LZWCommonDefs::Dictionary::Iterator::getNext() {
	outItem = nextItem;
	fetch();
	return outItem;

}

void LZWCommonDefs::Dictionary::Iterator::fetch() {
	if (nextItem.second != 0) {
		nextItem.second = nextItem.second->nextItem;
	}
	if (nextItem.second == 0) {
		nextItem.first++;
		while (nextItem.first < items.length() && items[nextItem.first] == null) {
			nextItem.first++;
		}
		if (nextItem.first < items.length()) {
			nextItem.second = items[nextItem.first];
		}
	}
}


}
