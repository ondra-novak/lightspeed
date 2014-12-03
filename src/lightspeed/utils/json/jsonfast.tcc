/*
 * jsonfast.tcc
 *
 *  Created on: 20.5.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_JSONFAST_TCC_
#define LIGHTSPEED_UTILS_JSONFAST_TCC_
#include "../../base/containers/constStr.h"
#include "../../base/containers/stringpool.tcc"
#include "../../base/containers/set.h"
#include "../../base/text/textParser.tcc"
#include "../../base/memory/allocPointer.h"
#include "../../base/text/textstream.tcc"
#include "jsonexception.h"
#include "jsonfast.h"
#include <cwctype>
#include "../../mt/fastlock.h"
#include "../../base/sync/synchronize.h"
#include "../../base/text/textParser.tcc"

namespace LightSpeed {
namespace JSON {



class FPool_t: public IFPool {
public:
	FPool_t(IRuntimeAlloc &alloc):alloc(alloc) {}
	~FPool_t() {}
	virtual IRuntimeAlloc &getAlloc() {
		return alloc;
	}
	virtual StrRefW_t addString(ConstStrW str) {
		return pool.add(str);
	}
	virtual StrRefA_t addString(ConstStrA str) {
		return apool.add(str);
	}
	virtual PNode getBoolNode(bool v) {
		if (v) {
			if (trueNode == nil) trueNode = new(alloc) DynNode_t<Bool_t>(true);
			return trueNode;
		} else {
			if (falseNode == nil) falseNode = new(alloc) DynNode_t<Bool_t>(false);
			return falseNode;
		}

	}
	virtual PNode getNullNode() {
		if (nullNode == nil) nullNode = new(alloc) DynNode_t<Null_t>();
		return nullNode;
	}

	class EmptyStrNode_t: public Null_t {
	public:
		virtual NodeType getType() const {return ndString;}
	};

	virtual PNode getEmptyStrNode() {
		if (emptyStrNode == nil) emptyStrNode = new(alloc) DynNode_t<EmptyStrNode_t>();
		return nullNode;
	}

	virtual void convertStr(const StrRefW_t &src, StringA &trg) {
		Synchronized<FastLock> _lk(convertLock);
		if (!trg.empty() || src.empty()) return;
		trg = String::getUtf8(src);
	}

	virtual void convertStr(const StrRefA_t &src, StringW &trg, bool ) {
		Synchronized<FastLock> _lk(convertLock);
		if (!trg.empty() || src.empty()) return;
		trg = String(ConstStrA(src));
	}

protected:
	StringPoolW pool;
	StringPoolA apool;
	IRuntimeAlloc &alloc;
	PNode trueNode;
	PNode falseNode;
	PNode nullNode;
	PNode emptyStrNode;
	FastLock convertLock;

};



template<typename T>
class ParserFast {
public:


	ParserFast(IIterator<char, T> &iter, const PFPool &pool):pool(pool),iter(iter),storedChar(0) {}
	PNode parseString();
	PNode parseObject();
	PNode parseArray();
	PNode parseValue(char firstChar);
	void parseCheck(const char *str);


	PNode parse();
	PNode parse(char firstChar);

protected:
	AutoArray<char> strBuff;
	PFPool pool;
	IIterator<char, T> &iter;
	char storedChar;

	void parseRawString();
	char getNextEatW();
	char getNextWithStored();
	ConstStrA arrayIndexStr(natural i);

	char getNextChr();
};


template<typename T>
PNode ParserFast<T>::parse() {
	try {
		char x = getNextEatW();
		return parse(x);
	} catch (Exception &e) {
		throw ParseError_t(THISLOCATION,"<root>") << e;
	}
}

template<typename T>
PNode ParserFast<T>::parse(char x) {
	switch (x) {
	case '"': return parseString();
	case '{': return parseObject();
	case '[': return parseArray();
	case 't': parseCheck(strTrue+1);
			  return pool->getBoolNode(true);
	case 'f': parseCheck(strFalse+1);
			  return pool->getBoolNode(false);
	case 'n': parseCheck(strNull+1);
			  return pool->getNullNode();
	default: return parseValue(x);
	}

}

template<typename T>
inline PNode ParserFast<T>::parseString() {
	parseRawString();
	PNode nd = new FRawValueW_t(pool,pool->addString(strBuff),true);
	return nd;

}

template<typename T>
char ParserFast<T>::getNextEatW() {
		char x = iter.getNext();
		while (iswspace(x)) x = iter.getNext();
		return x;
}

template<typename T>
char ParserFast<T>::getNextWithStored() {
	if (storedChar != 0) {
		char x = storedChar;
		storedChar = 0;
		while (iswspace(x)) x = getNextEatW();
		return x;
	} else {
		return getNextEatW();
	}
}

template<typename T>
inline char ParserFast<T>::getNextChr() {
	if (iter.hasItems()) return iter.getNext();
	else return 0;
}

template<typename T>
inline void ParserFast<T>::parseRawString() {
	char x = iter.getNext();
		strBuff.clear();
		while (x != '"') {
			if (x != '\\') {
				strBuff.add(x);
			} else {
				x = iter.getNext();
				char y;

				switch (x) {
					case '"': y = x;break;
					case '\\':y = x;break;
					case '/': y = x ;break;
					case 'b': y  = '\b';break;
					case 'f': y = '\f';break;
					case 'n': y = '\n'; break;
					case 'r': y = '\r';break;
					case 't': y = '\t';break;
					case 'u': {
							char buff[5];
							buff[4] = 0;
							for (int i = 0; i < 4; i++) {
								char z  = iter.getNext();
								if ((z >= '0' && z <= '9')||(z >='A' && z <= 'F') || (z >= 'a' && z <= 'f')) {
									buff[i] = (char)z;
								} else {
									throw ParseError_t(THISLOCATION,strBuff);
								}
							}
							wchar_t wy = (wchar_t)_intr::stringToUnsignedNumber<natural>(buff,16);
							WideToUtf8Filter flt;
							flt.input(wy);
							while (flt.hasItems()) strBuff.add(flt.output());
							x = iter.getNext();
							continue;
						}
					default: throw ErrorMessageException(THISLOCATION, "Neocekavana escape sekvence");
					//TODO: vymyslet lepsi vyjimku
				}
				strBuff.add(y);
			}
			x = iter.getNext();
		}
}


template<typename T>
inline PNode ParserFast<T>::parseObject() {
	char x = getNextEatW();
	AllocPointer<FObject_t> obj = new FObject_t(pool);
	while (x != '}') {
		if (x != '"')
			throw ParseError_t(THISLOCATION, "{");
		parseRawString();
		StrRefA_t name = pool->addString(strBuff);
		x = getNextEatW();
		if (x != ':')
			throw ParseError_t(THISLOCATION, name);
		try {
			PNode nd = parse(getNextEatW());
			obj->insertField(name,nd);
		} catch (Exception &e) {
			throw ParseError_t(THISLOCATION,name) << e;
		}
		x = getNextWithStored();
		if (x == ',')
			x = getNextEatW();
		else if (x != '}')
			throw ParseError_t(THISLOCATION,name);
	}
	return obj.detach();
}

template<typename T>
inline PNode ParserFast<T>::parseArray() {
	char x = getNextEatW();
	AllocPointer<Array_t> obj = new Array_t;
	natural i = 0;
	while (x != ']') {
		try {
			PNode nd = parse(x);
			obj->add(nd);
		} catch (Exception &e) {
			throw ParseError_t(THISLOCATION,arrayIndexStr(i)) << e;
		}
		i++;
		x = getNextWithStored();
		if (x == ',') x = getNextEatW();
		else if (x != ']')
			throw ParseError_t(THISLOCATION,arrayIndexStr(i));
	}
	return obj.detach();

}


template<typename T>
inline PNode ParserFast<T>::parseValue(char firstChar) {
	strBuff.clear();
	char x = firstChar;
	if (x == '-') {
		strBuff.add(x);
		x = getNextChr();
		storedChar = x;
	}
	if (x >= '1' && x <='9') {
		strBuff.add(x);
		x = getNextChr(); //x = iter.data[iter.ukazatel++]
		while (iswdigit(x)) {
			strBuff.add(x);
			x = getNextChr();
		}
		storedChar = x;
	} else if (x == '0') {
		strBuff.add(x);
		x = getNextChr();
		storedChar = x;
	} else {
		throw ParseError_t(THISLOCATION,strBuff);
	}
	if (x == '.') {
		strBuff.add(x);
		x = getNextChr();
		if (!iswdigit(x))
			throw ParseError_t(THISLOCATION,strBuff);
		while (iswdigit(x)) {
			strBuff.add(x);
			x = getNextChr();
		}
	}
	storedChar = x;

	if (x == 'E' || x == 'e') {
			strBuff.add(x);
			x = getNextChr();
			if (x == '-' || x == '+') {
				strBuff.add(x);
				x = getNextChr();
			}
			while (iswdigit(x)) {
				strBuff.add(x);
				x = getNextChr();
			}
			storedChar = x;
		}


	PNode nd = new FRawValueW_t(pool,pool->addString(strBuff),false);
	return nd;
}


template<typename T>
inline void ParserFast<T>::parseCheck(const char* str) {
	while (*str != 0) {
		char x = iter.getNext();
		if (x != *str) throw ParseError_t(THISLOCATION,"Unexpected character");
		str++;
	}
}

template<typename T>
PNode parseFast(IIterator<char, T> &iter, const PFPool &pool) {
	ParserFast<T> p(iter,pool);
	return p.parse();
}


template<typename T>
PNode parseFast(IIterator<char, T> &iter) {
	StdAlloc &s = StdAlloc::getInstance();
	return parseFast(iter,s);
}
template<typename T>
PNode parseFast(IIterator<char, T> &iter, IRuntimeAlloc &factory ) {
	PFPool pool = new FPool_t(factory);
	return parseFast(iter,pool);
}


template<typename T>
inline ConstStrA ParserFast<T>::arrayIndexStr(natural i) {
	char buff[100];
	strBuff.clear();
	strBuff.add('[');
	strBuff.append(_intr::numberToString(i,buff,100,10));
	strBuff.add(']');
	return strBuff;
}

template<typename T>
void serializeArray(const INode *json, IWriteIterator<char, T> &witer,bool escapeUTF8) {
	witer.write('[');
	Iterator iter = json->getFwIter();
	if (iter.hasItems()) {
		const NodeInfo &n = iter.getNext();
		serialize(n.node,witer,escapeUTF8);
		while (iter.hasItems()) {
			witer.write(',');
			const NodeInfo &n = iter.getNext();
			serialize(n.node,witer,escapeUTF8);
		}
	}
	witer.write(']');
}

template<typename T>
void serializeBool(const INode *json, IWriteIterator<char, T> &iter) {
	if (json->getBool()) {
		iter.blockWrite(ConstStrA(strTrue));
	} else {
		iter.blockWrite(ConstStrA(strFalse));
	}
}

template<typename T>
void serializeDeleted(const INode *, IWriteIterator<char, T> &iter) {
		iter.blockWrite(ConstStrA(strDelete));
}

template<typename T>
void serializeNull(const INode *, IWriteIterator<char, T> &iter) {
		iter.blockWrite(ConstStrA(strNull));
}


template<typename X, typename T>
void writeChar(X c, IWriteIterator<char, T> &iter) {
	switch (c) {
	case '\r': iter.write('\\');iter.write('r');break;
	case '\f': iter.write('\\');iter.write('f');break;
	case '\n': iter.write('\\');iter.write('n');break;
	case '\t': iter.write('\\');iter.write('t');break;
	case '\b': iter.write('\\');iter.write('b');break;
	case '\\': iter.write('\\');iter.write('\\');break;
	case '"': iter.write('\\');iter.write('\"');break;
	default:
		if (c>=0 && c < 32) {
			TextOut<IWriteIterator<char, T> &, StaticAlloc<32> > fmt(iter);
			fmt.setBase(16);
			fmt("\\u%{04}1") << (unsigned int)c;
		} else {
			iter.write(c);
		}
	}
}
template<typename T>
void serializeString(ConstStrA str, IWriteIterator<char, T> &witer) {

	witer.write('"');
	for (ConstStrA::Iterator iter = str.getFwIter(); iter.hasItems();) {
		writeChar<char,T>(iter.getNext(),witer);
	}
	witer.write('"');

}

template<typename T>
void serializeString(ConstStrW str, IWriteIterator<char, T> &witer) {
	witer.write('"');
	for (ConstStrW::Iterator iter = str.getFwIter(); iter.hasItems();) {
		wchar_t c = iter.getNext();
		if (c > 0x7F) {
			TextOut<IWriteIterator<char, T> &, StaticAlloc<32> > fmt(witer);
			fmt.setBase(16);
			fmt("\\u%{04}1") << (unsigned int)c;
		} else {
			writeChar<char,T>((char)c,witer);
		}
	}
	witer.write('"');
}

template<typename T>
void serializeStringEscUTF(ConstStrA str, IWriteIterator<char, T> &witer) {
	witer.write('"');
	for (ConstStrA::Iterator iter = str.getFwIter(); iter.hasItems();) {
		signed char c = iter.getNext();
		if (c < 0) {
			Utf8ToWideFilter flt;
			flt.input(c);
			while (!flt.hasItems()) flt.input(iter.getNext());
			flt.flush();
			while (flt.hasItems()) {
				wchar_t cu = flt.output();
				TextOut<IWriteIterator<char, T> &, StaticAlloc<32> > fmt(witer);
				fmt.setBase(16);
				fmt("\\u%{04}1") << (unsigned int)cu;
			}
		} else {
			writeChar<char,T>(c,witer);
		}
	}
	witer.write('"');
}


template<typename T>
void serializeObjectNode(const NodeInfo &n, IWriteIterator<char, T> &iter, bool escapeUTF8) {
	if (escapeUTF8)
		serializeStringEscUTF(n.key->getString(),iter);
	else
		serializeString(n.key->getString(),iter);

	iter.write(':');
	serialize(n.node,iter,escapeUTF8);
}


template<typename T>
void serializeObject(const INode *json, IWriteIterator<char, T> &witer, bool escapeUTF8) {
	witer.write('{');
	Iterator iter = json->getFwIter();
	if (iter.hasItems()) {
		const NodeInfo &n = iter.getNext();
		serializeObjectNode(n,witer,escapeUTF8);
		while (iter.hasItems()) {
			witer.write(',');
			const NodeInfo &n = iter.getNext();
			serializeObjectNode(n,witer,escapeUTF8);
		}
	}
	witer.write('}');
}


template<typename T>
void serializeString(const INode *json, IWriteIterator<char, T> &witer, bool escapeUTF8) {

	if (json->isUtf8()) {
		if (escapeUTF8)
			serializeStringEscUTF(json->getStringUtf8(),witer);
		else
			serializeString(json->getStringUtf8(),witer);
	}else
		serializeString(json->getString(), witer);
}

template<typename T>
void serializeFloat(const INode *json, IWriteIterator<char, T> &witer) {

	TextOut<IWriteIterator<char, T> &, StaticAlloc<140> > fmt(witer);
	fmt() << json->getFloat();
}


template<typename T>
void serializeInt(const INode *json, IWriteIterator<char, T> &witer) {

	TextOut<IWriteIterator<char, T> &, StaticAlloc<140> > fmt(witer);
	fmt() << json->getInt();
}

template<typename T>
void serialize(const INode *json, IWriteIterator<char, T> &iter, bool escapeUTF8) {
	if (json == 0) throwNullPointerException(THISLOCATION);
	NodeType t = json->getType();
	switch (t) {
	case ndArray: serializeArray(json,iter,escapeUTF8); break;
	case ndBool: serializeBool(json,iter); break;
	case ndObject: serializeObject(json,iter,escapeUTF8);break;
	case ndDelete: serializeDeleted(json,iter);break;
	case ndFloat: serializeFloat(json,iter);break;
	case ndInt: serializeInt(json,iter);break;
	case ndNull: serializeNull(json,iter);break;
	case ndString: serializeString(json,iter,escapeUTF8);break;
	}
}



}
}


#endif /* LIGHTSPEED_UTILS_JSONFAST_TCC_ */
