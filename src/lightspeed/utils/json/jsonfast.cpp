/*
 * jsonfast.cpp
 *
 *  Created on: 20.5.2013
 *      Author: ondra
 */

#include "../../base/containers/constStr.h"
#include "../../base/memory/refCntPtr.h"

#include "jsonfast.h"
#include "../../base/text/textParser.h"
#include <math.h>
#include "../../base/exceptions/invalidNumberFormat.h"
#include "../../base/memory/smallAlloc.h"
#include <string.h>
#include "../../mt/atomic.h"
#include "../../base/text/textFormat.h"
#include "jsonfast.tcc"

namespace LightSpeed {namespace JSON {

const char *strTrue = "true";
const char *strFalse = "false";
const char *strNull = "null";
const char *strDelete = "deleted";

#ifdef _WIN32
static double round(double v) {
	return floor(v + 0.5);
}
#endif

class StrKeyA: public IKey {
public:
	StrKeyA() {}
	StrKeyA(ConstStrA a):str(a) {}

	StrKeyA &operator= (const StrKeyA &other) {
		if (this != &other) {
			str = other.str;
			buffer.clear();
		}
		return *this;
	}

	virtual Type getType() const {return string;}
	virtual ConstStrA getString() const {return str;}
	virtual natural getIndex() const {
		natural res = 0;
		parseUnsignedNumber(str.getFwIter(),res,10);
		return res;
	}


protected:
	mutable AutoArray<wchar_t> buffer;
	ConstStrA str;
};

integer FRawValueW_t::getInt() const  {
	TextParser<char, SmallAlloc<256> > parser;
	if (parser("%d1",value)) {
		return (integer)parser[1];
	} else if (parser("%f1",value)) {
		return (integer)round(parser[1]);
	} else
		throw InvalidNumberFormatException(THISLOCATION, ConstStrA(value));
}

natural FRawValueW_t::getUInt() const {
	TextParser<char, SmallAlloc<256> > parser;
	if (parser("%u1",value)) {
		return (natural)parser[1];
	} else if (parser("%f1",value)) {
		return (natural)round(parser[1]);
	} else
		throw InvalidNumberFormatException(THISLOCATION, ConstStrA(value));
}

double FRawValueW_t::getFloat() const {
	TextParser<char, SmallAlloc<256> > parser;
	if (parser("%f1",value)) {
		return parser[1];
	} else
		throw InvalidNumberFormatException(THISLOCATION, ConstStrA(value));
}

bool FRawValueW_t::getBool() const {
	if (value == ConstStrA(strFalse) || nodeType == ndNull || value == ConstStrA("0") ) return false;
	return true;
}

INode* FRawValueW_t::clone(PFactory factory) const {
	switch (nodeType) {
	case ndBool: return factory->newValue(getBool()).detach();
	case ndFloat: return factory->newValue(getFloat()).detach();
	case ndInt: return factory->newValue(getInt()).detach();
	case ndNull: return factory->newNullNode().detach();
	default: return factory->newValue(ConstStrA(value)).detach();
	}
}

INode& FRawValueW_t::operator [](ConstStrW ) const {
	throwUnsupportedFeature(THISLOCATION,this,"operator[]");throw;
}

INode& FRawValueW_t::operator [](ConstStrA ) const {
	throwUnsupportedFeature(THISLOCATION,this,"operator[]");throw;
}

INode& FRawValueW_t::operator [](natural ) const {
	throwUnsupportedFeature(THISLOCATION,this,"operator[]");throw;
}

ConstStrW FRawValueW_t::getString() const {
	if (wvalue.empty()) {
		f->convertStr(value,wvalue,false);
	}
	return wvalue;
}

INode* FRawValueW_t::add(PNode ) { throwUnsupportedFeature(THISLOCATION,this,"add");return this;}
INode* FRawValueW_t::add(ConstStrW , PNode ) {throwUnsupportedFeature(THISLOCATION,this,"add");return this;}
INode* FRawValueW_t::add(ConstStrA , PNode ) {throwUnsupportedFeature(THISLOCATION,this,"add");return this;}
INode* FRawValueW_t::erase(ConstStrW ) {throwUnsupportedFeature(THISLOCATION,this,"add");return this;}
INode* FRawValueW_t::erase(ConstStrA ) {throwUnsupportedFeature(THISLOCATION,this,"add");return this;}
INode* FRawValueW_t::erase(natural ) {throwUnsupportedFeature(THISLOCATION,this,"add");return this;}

NodeType FRawValueW_t::detectType(const StrRefA_t& text, bool str)  {
	if (str) return ndString;
	else if (text == ConstStrA(strTrue) || text == ConstStrA(strFalse)) return ndBool;
	else if (text == ConstStrA(strNull)) return ndNull;
	else if (text.find('.') != naturalNull || text.find('e') != naturalNull || text.find('E') != naturalNull)
		return ndFloat;
	else
		return ndInt;
}


void FObject_t::insertField(const StrRefA_t& name, PNode nd) {
	if (isMTAccessEnabled()) fields.replace(name,nd.getMT());
	fields.replace(name,nd);
}

INode* FObject_t::getVariable(ConstStrW var) const {

	WideToUtf8Reader<ConstStrW::Iterator> rd(var.getFwIter());
	AutoArray<char,SmallAlloc<256> > buff(rd);
	const PNode *x = fields.find(ConstStrA(buff));
	if (x) return *x; else return 0;
}

bool FObject_t::enumEntries(const IEntryEnum& fn) const {
	StrKeyA k;
	for (FFieldMap_t::Iterator iter = fields.getFwIter();iter.hasItems();) {
		const FFieldMap_t::Entity &e = iter.getNext();
		k = StrKeyA(e.key);
		if (fn(*e.value.get(),k)) return true;
	}
	return false;
}

INode* FObject_t::add(PNode nd) {
	char buff[100];
	_intr::numberToString(length(),buff+1,99,10);
	buff[0] = '_';
	return add(ConstStrA(buff),nd);
}

INode* FObject_t::add(ConstStrW name, PNode nd) {
	if (nd == nil) nd = getNullNode();
	IFPool *ff = static_cast<IFPool *>(f.get());
	WideToUtf8Reader<ConstStrW::Iterator> rd(name.getFwIter());
	AutoArray<char,SmallAlloc<256> > buff(rd);
	StrRefA_t str = ff->addString(buff);
	insertField(str,nd);
	return this;
}

INode* FObject_t::enableMTAccess() {
	RefCntObj::enableMTAccess();
	for (FFieldMap_t::Iterator iter = fields.getFwIter(); iter.hasItems();) {
		const FFieldMap_t::Entity &e = iter.getNext();
		e.value->enableMTAccess();
	}
	return this;
}


INode* FObject_t::clone(PFactory factory) const {
	PNode r = factory->newClass();
	for (FFieldMap_t::Iterator iter = fields.getFwIter();iter.hasItems();) {
		const FFieldMap_t::Entity &e = iter.getNext();
		r->add(e.key,e.value->clone(factory));
	}
	return r.detach();
}

bool FObject_t::operator ==(const INode& other) const {
	const FObject_t *k = dynamic_cast<const FObject_t *>(&other);
	if (k == 0) return false;
	if (fields.length() != k->fields.length()) return false;
	for (FFieldMap_t::Iterator iter = fields.getFwIter(),iter2 =k->fields.getFwIter();
		iter.hasItems();) {
		const FFieldMap_t::Entity &e1 = iter.getNext();
		const FFieldMap_t::Entity &e2 = iter2.getNext();
		if (e1.key != e2.key || (*e1.value) != (*e2.value)) return false;
	}
	return true;
}




Iterator FObject_t::getFwIter() const {

	class Iter: public Iterator::IIntIter, public DynObject {
	public:
		Iter(const FFieldMap_t &mp):iter(mp.getFwIter()) {}

		virtual bool hasItems() const {return iter.hasItems();}
		virtual bool getNext(NodeInfo &nfo) {
			const FFieldMap_t::Entity &e = iter.getNext();
			k = ConstStrA(e.key);
			nfo.key = &k;
			nfo.node = e.value;
			return iter.hasItems();
		}
		virtual bool peek(NodeInfo &nfo) const {
			const FFieldMap_t::Entity &e = iter.peek();
			k = ConstStrA(e.key);
			nfo.key = &k;
			nfo.node = e.value;
			return iter.hasItems();
		}
		virtual IIntIter *clone(IRuntimeAlloc &alloc) const {
			return new(alloc) Iter(*this);
		}

	protected:
		FFieldMap_t::Iterator iter;
		mutable StrKeyA k;
	};

	Iter x(fields);
	return Iterator(&x);

}

Iterator FRawValueW_t::getFwIter() const {
	return Iterator();
}

natural FObject_t::getUInt() const {
	return 0;
}
INode *FObject_t::getVariable(ConstStrA s) const {
	const PNode *x = fields.find(s);
	if (x) return *x; else return 0;
}
INode &FObject_t::operator[](ConstStrW v) const {
	INode *x = getVariable(v);
	if (x == 0) throw RequiredFieldException(THISLOCATION,v);
	return *x;
}
INode &FObject_t::operator[](ConstStrA v) const {
	INode *x = getVariable(v);
	if (x == 0) throw RequiredFieldException(THISLOCATION,v);
	return *x;
}
INode &FObject_t::operator[](natural index) const {
	TextFormatBuff<char,StaticAlloc<100> > fmt;
	fmt("%1") << index;
	INode *x = getVariable(fmt.write());
	if (x == 0) throw RequiredFieldException(THISLOCATION,fmt.getBuffer());
	return *x;
}
ConstStrA FObject_t::getStringUtf8() const {
	return ConstStrA();
}
INode *FObject_t::add(ConstStrA name, PNode nd) {
	if (nd == nil) nd = getNullNode();
	StrRefA_t str = f->addString(name);
	insertField(str,nd);
	return this;
}
INode* FObject_t::erase(ConstStrA name) {
	fields.erase(name);
	return this;
}

INode* FObject_t::erase(ConstStrW name) {
	WideToUtf8Reader<ConstStrW::Iterator> rd(name.getFwIter());
	AutoArray<char,SmallAlloc<256> > buff(rd);
	fields.erase(ConstStrA(buff));
	return this;
}


void toStream(const INode *json, SeqFileOutput &output, bool esc) {

	SeqTextOutA f(output);
	serialize(json,f,esc);

}

LightSpeed::StringA toString( const INode *json, bool escapeUTF8 )
{
	AutoArrayStream<char> buff;
	serialize(json,buff,escapeUTF8);
	return buff.getArray();
}

PNode fromStream(SeqFileInput& input) {
	SeqTextInA txt(input);
	return parseFast(txt);
}


}

}

