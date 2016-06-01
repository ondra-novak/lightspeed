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



class AbstractNode_t: public INode {
public:
	using INode::add;
	using INode::erase;
	using INode::operator[];
    virtual natural getUInt() const {return (natural)getInt();}
	virtual lnatural getLongUInt() const {return (lnatural)getLongInt();}

	LightSpeed::ConstStrA getStringUtf8() const;
	LightSpeed::ConstStrW getString() const;
	INode & operator[]( ConstStrA v ) const;
	INode & operator[]( natural index ) const;

	virtual INode *replace(ConstStrA , Value , Value *prevValue = 0) {
		if (prevValue) *prevValue = null;
		return this;
	}
	virtual INode *replace(natural , Value , Value *prevValue = 0) {
		if (prevValue) *prevValue = null;
		return this;
	};
	virtual INode *clear() {return this;}
	virtual bool empty() const {return false;}



	bool isUtf8() const {return false;}
	virtual Value copy(PFactory , natural , bool mt_share) const;



};

class LeafNode: public AbstractNode_t {
public:
	virtual INode *getVariable(ConstStrA) const {return 0;}
	virtual natural getEntryCount() const {return 0;}
	virtual INode *getEntry(natural idx) const {return idx>0?0:const_cast<LeafNode *>(this);}
	virtual bool enumEntries(const IEntryEnum &) const {return true;}
	virtual INode *add(PNode) {
		throwUnsupportedFeature(THISLOCATION,this,"Object is not container");
		return this;
	}
	virtual INode *add(ConstStrA , PNode nd) {
		return add(nd);
	}
	virtual INode*  erase(natural ) {return this;}
	virtual INode* erase(ConstStrA) {return this;}

	virtual const INode*  enableMTAccess() const {
		RefCntObj::enableMTAccess();
		return this;
	}

};

class Object;

class FactoryCommon: public IFactory, public IFactoryToStringProperty {
public:

	///Create object (obsolete)
	virtual Value newClass() {return newObject();}
	///Creates JSON object
	virtual Value newObject();
	///Creates JSON array
	virtual Value newArray();
	///Creates JSON number using unsigned value
	virtual Value newValue(natural v);
	///Creates JSON number using signed value
	virtual Value newValue(integer v);
#ifdef LIGHTSPEED_HAS_LONG_TYPES
	///Creates JSON number using unsigned value
	virtual Value newValue(lnatural v);
	///Creates JSON number using signed value
	virtual Value newValue(linteger v);
#endif
	///Creates JSON number using double-float value
	virtual Value newValue(double v);
	///Creates JSON bool and stores value
	virtual Value newValue(bool v);
	///Creates JSON string
	virtual Value newValue(ConstStrW v);
	///Creates JSON string
	virtual Value newValue(ConstStrA v);
	///Creates JSON array
	virtual Value newValue(ConstStringT<JSON::Value> v);
	///Creates JSON array
	virtual Value newValue(ConstStringT<JSON::INode *> v);

	virtual Value newValue(NullType);


	using IFactory::newValue;

	virtual PNode newValue(const char *v) {return newValue(ConstStrA(v));}
	virtual PNode newValue(const wchar_t *v) {return newValue(ConstStrW(v));}


	virtual void toStream(const INode &nd, SeqFileOutput &stream);
	virtual void enableUTFEscaping(bool enable) {escapeUTF = enable;}

	LightSpeed::JSON::PNode mergeClasses( const Object * clschange, const Object * clsbase );

	bool escapeUTF;

	FactoryCommon():escapeUTF(false) {}

protected:
	///Creates JSON object
	virtual Value createObject() = 0;
	///Creates JSON number using unsigned value
	virtual Value createNumber(natural v) = 0;
	///Creates JSON number using signed value
	virtual Value createNumber(integer v) = 0;
#ifdef LIGHTSPEED_HAS_LONG_TYPES
	///Creates JSON number using unsigned value
	virtual Value createNumber(lnatural v) = 0;
	///Creates JSON number using signed value
	virtual Value createNumber(linteger v) = 0;
#endif
	///Creates JSON number using double-float value
	virtual Value createNumber(double v) = 0;
	///Creates JSON bool and stores value
	virtual Value createNumber(bool v) = 0;
	///Creates JSON string
	virtual Value createString(ConstStrW v) = 0;
	///Creates JSON string
	virtual Value createString(ConstStrA v) = 0;
	///Creates JSON array
	virtual Value createArray(ConstStringT<JSON::Value> v) = 0;
	///Creates JSON array
	virtual Value createArray(ConstStringT<JSON::INode *> v) = 0;

};

class Factory: public FactoryCommon {
public:
	using FactoryCommon::newValue;
	virtual PNode createObject();
	virtual PNode createNumber(natural v);
	virtual PNode createNumber(integer v);
#ifdef LIGHTSPEED_HAS_LONG_TYPES
	virtual PNode createNumber(lnatural v);
	virtual PNode createNumber(linteger v);
#endif
	virtual PNode createNumber(float v);
	virtual PNode createNumber(double v);
	virtual PNode createNumber(bool v);
	virtual PNode createString(ConstStrW v);
	virtual PNode createString(ConstStrA v);
	virtual Value createArray(ConstStringT<JSON::Value> v);
	virtual Value createArray(ConstStringT<JSON::INode *> v);


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
	virtual Value fromStream( SeqFileInput &stream );
	virtual Value fromCharStream( IVtIterator<char> &iter);
	virtual IRuntimeAlloc *getAllocator() const {return &StdAlloc::getInstance();}
	virtual IFactory *clone() {return new Factory;}

	template<typename T>
	Value parseStream(IIterator<char,T> &iter);
};


}

}


#endif /* LIGHTSPEED_UTILS_JSONDEFS_H_ */
