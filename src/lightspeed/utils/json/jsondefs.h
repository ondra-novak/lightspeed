/*
 * jsondefs.h
 *
 *  Created on: Jan 28, 2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_JSONDEFS_H_
#define LIGHTSPEED_UTILS_JSONDEFS_H_
#include "../../base/text/textIn.h"
#include "../../base/streams/utf.h"
#include "../../base/memory/smallAlloc.h"


#pragma once

namespace LightSpeed {


namespace JSON {

String decodeString( ConstStrW jsontext );

PNode getNullNode();
PNode getDeleteNode();

typedef TextIn<Utf8ToWideReader<ConstStrA::Iterator>, SmallAlloc<256> > JSONStream_t;

typedef TextIn<Utf8ToWideReader<ConstStrA::Iterator>, SmallAlloc<256> > JSONStream_t;

template<typename JSONStream_t>
PNode JSON_parseNode( JSONStream_t &parser, PFactory factory );

class NumKey: public IKey {
public:
	NumKey() {}
	NumKey(natural i):i(i) {}

	virtual Type getType() const {return index;}
	virtual ConstStrA getString() const {return ConstStrA();}
	virtual natural getIndex() const {return i;}
protected:
	natural i;
};


class AbstractNode_t: public INode {
public:
	using INode::add;
	using INode::erase;
	using INode::operator[];
    virtual natural getUInt() const {return (natural)getInt();}

	LightSpeed::ConstStrA getStringUtf8() const;
	INode & operator[]( ConstStrA v ) const;
	INode & operator[]( natural index ) const;

	bool isUtf8() const {return false;}


};

class LeafNode_t: public AbstractNode_t {
public:
	virtual INode *getVariable(ConstStrA) const {return 0;}
	virtual natural getEntryCount() const {return 0;}
	virtual INode *getEntry(natural idx) const {return idx>0?0:const_cast<LeafNode_t *>(this);}
	virtual bool enumEntries(const IEntryEnum &fn) const {
		return fn(*this,NumKey(0));
	}
	virtual INode *add(PNode) {
		throwUnsupportedFeature(THISLOCATION,this,"Object is not container");
		return this;
	}
	virtual INode *add(ConstStrA , PNode nd) {
		return add(nd);
	}
	virtual INode*  erase(natural ) {return this;}
	virtual INode* erase(ConstStrA) {return this;}

	virtual INode*  enableMTAccess() {
		RefCntObj::enableMTAccess();
		return this;
	}

	virtual Iterator getFwIter() const {return Iterator();}
};

class Object_t;

class FactoryCommon_t: public IFactory, public IFactoryToStringProperty {
public:
	virtual PNode newNullNode() {return getNullNode();}
	virtual PNode newDeleteNode() {return getDeleteNode();}
	virtual PNode merge(const INode &base, const INode &change);

	using IFactory::newValue;

	virtual PNode newValue(const char *v) {return newValue(ConstStrA(v));}
	virtual PNode newValue(const wchar_t *v) {return newValue(ConstStrW(v));}


	virtual void toStream(const INode &nd, SeqFileOutput &stream);
	virtual void enableUTFEscaping(bool enable) {escapeUTF = enable;}

	LightSpeed::JSON::PNode mergeClasses( const Object_t * clschange, const Object_t * clsbase );

	bool escapeUTF;

	FactoryCommon_t():escapeUTF(false) {}
};

class Factory_t: public FactoryCommon_t {
public:
	using FactoryCommon_t::newValue;
	virtual PNode newObject() {return newClass();}
	virtual PNode newClass();
	virtual PNode newArray();
	virtual PNode newValue(natural v);
	virtual PNode newValue(integer v);
	virtual PNode newValue(float v);
	virtual PNode newValue(double v);
	virtual PNode newValue(bool v);
	virtual PNode newValue(ConstStrW v);
	virtual PNode newValue(ConstStrA v);

	virtual ConstStrA toString(const INode &nd);

	typedef AutoArrayStream<char> Stream_t;

	static void buildResult( Stream_t &outStr, const INode &nd );
	static void writeString( Stream_t &outStr, ConstStrW str);
	static void writeString( Stream_t &outStr, ConstStrA str);

	static void buildStringResult( Stream_t & outStr, const INode &nd );

	static void buildIntResult( Stream_t & outStr, const INode &nd );

	static void buildFloatResult( Stream_t & outStr, const INode &nd );

	Stream_t strRes;

	virtual PNode fromString(ConstStrA text);
	virtual IRuntimeAlloc *getAllocator() const {return &StdAlloc::getInstance();}
	virtual IFactory *clone() {return new Factory_t;}
};

class Iterator::IIntIter{
public:

	virtual bool hasItems() const = 0;
	virtual bool getNext(NodeInfo &nfo) = 0;
	virtual bool peek(NodeInfo &nfo) const = 0;
	virtual IIntIter *clone(IRuntimeAlloc &alloc) const = 0;
	virtual ~IIntIter() {}
};

}

}


#endif /* LIGHTSPEED_UTILS_JSONDEFS_H_ */
