/*
 * jsonimpl.h
 *
 *  Created on: 20.5.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_JSONIMPL_H_
#define LIGHTSPEED_UTILS_JSONIMPL_H_
#include "../../base/types.h"
#include "../../base/containers/map.h"
#include "jsondefs.h"
#include "../../base/containers/stringparam.h"
#include "../../base/exceptions/errorMessageException.h"

namespace LightSpeed {

namespace JSON {

typedef Map<StrParamA,PNode> FieldMap_t;
class Object_t: public AbstractNode_t {
public:
	virtual NodeType getType() const {return ndObject;}
	virtual ConstStrW getString() const {return ConstStrW();}
	virtual integer getInt() const {return integerNull;}
	virtual linteger getLongInt() const {return integerNull;}
	virtual double getFloat() const {return 0;}
	virtual bool getBool() const {return true;}
	virtual bool isNull() const {return false;}
	virtual INode *getVariable(ConstStrA var) const;
	virtual natural getEntryCount() const {return 0;}
	virtual INode *getEntry(natural ) const {return 0;}
	virtual bool enumEntries(const IEntryEnum &fn) const;

	virtual bool empty() const {return fields.empty();}
	void insertField(const StringA &name, PNode nd);
	virtual INode *add(PNode nd);
	virtual INode *add(ConstStrA name, PNode nd);

	virtual INode * erase(ConstStrA name) {fields.erase(name);return this;}
	virtual INode *erase(natural ) {return this;}

	virtual INode * enableMTAccess();

	virtual INode *clone(PFactory factory) const;

	virtual bool operator==(const INode &other) const;

	virtual Iterator getFwIter() const;

protected:
	FieldMap_t fields;
};


typedef AutoArray<PNode> FieldList_t;
class Array_t: public AbstractNode_t {
public:
	virtual NodeType getType() const {return ndArray;}
	virtual ConstStrW getString() const {return list.empty()?ConstStrW():list[0]->getString();}
	virtual integer getInt() const {return list.empty()?0:list[0]->getInt();}
	virtual linteger getLongInt() const {return integerNull;}
	virtual double getFloat() const {return list.empty()?0:list[0]->getFloat();}
	virtual bool getBool() const {return list.empty()?0:list[0]->getBool();}
	virtual bool isNull() const {return list.empty()?0:list[0]->isNull();}
	virtual INode *getVariable(ConstStrA x)  const {return list.empty()?0:list[0]->getVariable(x);}
	virtual natural getEntryCount() const {return list.length();}
	virtual INode *getEntry(natural idx) const {return list[idx];}
	virtual bool empty() const {return list.empty();}
	virtual bool enumEntries(const IEntryEnum &fn) const;

	virtual INode *add(PNode nd);
	virtual INode *add(ConstStrA name, PNode nd);

	void internalAdd(PNode nd);
	virtual INode *erase(ConstStrA ) {return this;}
	virtual INode *erase(natural index);
	virtual INode *enableMTAccess();

	virtual INode *clone(PFactory factory) const;

	virtual bool operator==(const INode &other) const;

	virtual Iterator getFwIter() const;

protected:
	FieldList_t list;
};

class TextField_t: public LeafNode_t {
public:
	TextField_t(String x):x(x),utf(0) {}
	~TextField_t();

	virtual NodeType getType() const {return ndString;}
	virtual ConstStrW getString() const {return x;}
	virtual integer getInt() const;
	virtual linteger getLongInt() const;
	virtual double getFloat() const;
	virtual bool getBool() const {return !x.empty();}
	virtual bool isNull() const {return false;}
	ConstStrA getStringUtf8() const;
	virtual INode * enableMTAccess();
	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;

	String x;
	mutable StringA *utf;
};

class TextFieldA_t: public LeafNode_t {
public:
	TextFieldA_t(StringA x):x(x),unicode(0) {}
	~TextFieldA_t();

	virtual NodeType getType() const {return ndString;}
	virtual ConstStrW getString() const;
	virtual integer getInt() const;
	virtual linteger getLongInt() const;
	virtual double getFloat() const;
	virtual bool getBool() const {return !x.empty();}
	virtual bool isNull() const {return false;}
	ConstStrA getStringUtf8() const  {return x;}
	virtual INode * enableMTAccess();
	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;
	virtual bool isUtf8() const {return true;}

	StringA x;
	mutable String *unicode;
};


class IntField_t: public LeafNode_t {
public:
	IntField_t(integer x):x(x) {}
	virtual NodeType getType() const {return ndInt;}
	virtual ConstStrW getString() const;
	virtual integer getInt() const {return x;}
	virtual linteger getLongInt() const {return x;}
	virtual double getFloat() const {return double(x);}
	virtual bool getBool() const {return x != 0;}
	virtual bool isNull() const {return false;}
	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;

	integer x;
	mutable String strx;
};

class IntField64_t: public LeafNode_t {
public:
	IntField64_t(linteger x):x(x) {}
	virtual NodeType getType() const {return ndInt;}
	virtual ConstStrW getString() const;
	virtual integer getInt() const {return (integer)x;}
	virtual linteger getLongInt() const {return x;}
	virtual double getFloat() const {return double(x);}
	virtual bool getBool() const {return x != 0;}
	virtual bool isNull() const {return false;}
	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;

	linteger x;
	mutable String strx;
};

class FloatField_t: public LeafNode_t {
public:
	FloatField_t(double x):x(x) {}
	virtual NodeType getType() const {return ndFloat;}
	virtual ConstStrW getString() const;
	virtual integer getInt() const {return (integer)x;}
	virtual linteger getLongInt() const {return (linteger)x;}
	virtual double getFloat() const {return x;}
	virtual bool getBool() const {return x != 0;}
	virtual bool isNull() const {return false;}

	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;

	double x;
	mutable String strx;
};

class Null_t: public LeafNode_t {
public:
	virtual NodeType getType() const {return ndNull;}
	virtual ConstStrW getString() const {return String();}
	virtual integer getInt() const {return integerNull;}
	virtual linteger getLongInt() const {return lintegerNull;}
	virtual natural getUInt() const {return naturalNull;}
	virtual lnatural getLongUInt() const {return lnaturalNull;}
	virtual double getFloat() const {return 0;}
	virtual bool getBool() const {return false;}
	virtual bool isNull() const {return true;}

	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;
};
class Delete_t: public LeafNode_t {
public:
	virtual NodeType getType() const {return ndDelete;}
	virtual ConstStrW getString() const {throw accessError(THISLOCATION);}
	virtual integer getInt() const {throw accessError(THISLOCATION);}
	virtual linteger getLongInt() const {throw accessError(THISLOCATION);}
	virtual double getFloat() const {throw accessError(THISLOCATION);}
	virtual bool getBool() const {throw accessError(THISLOCATION);}
	virtual bool isNull() const {throw accessError(THISLOCATION);}

	virtual INode *clone(PFactory factory) const;

	virtual bool operator==(const INode &other) const;

	ErrorMessageException accessError(const ProgramLocation &loc) const;

};



class Bool_t: public LeafNode_t {
public:
	Bool_t(bool b):b(b) {}

	virtual NodeType getType() const {return ndBool;}
	virtual ConstStrW getString() const {return b?L"true":L"false";}
	virtual integer getInt() const {return b?naturalNull:0;}
	virtual linteger getLongInt() const {return b?naturalNull:0;}
	virtual double getFloat() const {return b?-1:0;}
	virtual bool getBool() const {return b;}
	virtual bool isNull() const {return false;}

	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;

	bool b;
};


template<typename T>
class DynNode_t: public T, public DynObject {
public:
	DynNode_t() {}

	template<typename X>
	DynNode_t(const X &x):T(x) {}
};

template<template<typename> class T = DynNode_t>
class FactoryAlloc_t: public Factory_t {
public:

	FactoryAlloc_t(IRuntimeAlloc  &alloc):alloc(alloc) {}

	virtual PNode newClass() {return new(alloc) T<Object_t>;}
	virtual PNode newArray() {return new(alloc) T<Array_t>;}
	virtual PNode newValue(natural v) {return new(alloc) T<IntField_t>(v);}
	virtual PNode newValue(integer v) {return new(alloc) T<IntField_t>(v);}
#ifdef LIGHTSPEED_HAS_LONG_TYPES
	virtual PNode newValue(lnatural v) {return new(alloc) T<IntField64_t>(v);}
	virtual PNode newValue(linteger v) {return new(alloc) T<IntField64_t>(v);}
#endif
	virtual PNode newValue(float v) {return new(alloc) T<FloatField_t>(v);}
	virtual PNode newValue(double v) {return new(alloc) T<FloatField_t>(v);}
	virtual PNode newValue(bool v) {return new(alloc) T<Bool_t>(v);}
	virtual PNode newValue(ConstStrW v) {return new(alloc) T<TextField_t>(v);}
	virtual PNode newValue(ConstStrA v) {return new(alloc) T<TextFieldA_t>(v);}
	virtual PNode newValue(const String &v) {return new(alloc) T<TextField_t>(v);}
	virtual PNode newValue(const StringA &v) {return new(alloc) T<TextField_t>(v);}
	virtual IRuntimeAlloc *getAllocator() const {return &alloc;}

	virtual IFactory *clone() {return new FactoryAlloc_t(alloc);}

protected:

	IRuntimeAlloc &alloc;
};

}
}



#endif /* LIGHTSPEED_UTILS_JSONIMPL_H_ */
