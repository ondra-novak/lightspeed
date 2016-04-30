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
class Object: public AbstractNode_t {
public:
	virtual NodeType getType() const {return ndObject;}
	virtual ConstStrW getString() const {return ConstStrW();}
	virtual integer getInt() const {return integerNull;}
	virtual linteger getLongInt() const {return integerNull;}
	virtual double getFloat() const {return 0;}
	virtual bool getBool() const {return true;}
	virtual bool isNull() const {return false;}
	virtual INode *getVariable(ConstStrA var) const;
	virtual natural getEntryCount() const {return fields.size();}
	virtual INode *getEntry(natural ) const {return 0;}
	virtual bool enumEntries(const IEntryEnum &fn) const;

	virtual bool empty() const {return fields.empty();}
	virtual natural length() const;
	void insertField(ConstStrA name, PNode nd);
	virtual INode *add(PNode nd);
	virtual INode *add(ConstStrA name, PNode nd);

	virtual INode * erase(ConstStrA name);
	virtual INode *erase(natural ) {return this;}

	virtual const INode * enableMTAccess() const ;

	virtual INode *clone(PFactory factory) const;

	virtual bool operator==(const INode &other) const;

	virtual INode *replace(ConstStrA name, Value newValue, Value *prevValue = 0);
	virtual INode *clear();

	Object();
	~Object();

protected:

	struct Field {
		ConstStrA key;
		mutable Value value;
		//string follows there....


		Field(ConstStrA key, Value value)
			:key(key),value(value) {}

		Field(const Field &other):key(other.key),value(other.value) {}
	};

	typedef AvlTreeNode<Field> FieldNode;
	class FieldNodeNew: public FieldNode {
	public:

		FieldNodeNew(const Field &d):FieldNode(d) {}

		void *operator new( size_t objSize, ConstStrA str, ConstStrA &stored);
		void operator delete(void *ptr, ConstStrA str, ConstStrA &stored);
		void operator delete(void *ptr, size_t sz);

	};

	static void releaseNode(FieldNode *x);
	struct CompareItems{bool operator()(const FieldNode *a, const FieldNode *b) const;};


	typedef AvlTreeBasic<CompareItems, Field > FieldMap;
	FieldMap fields;
};


typedef AutoArray<PNode> FieldList_t;
class Array: public AbstractNode_t {
public:
	Array();
	Array(ConstStringT<Value> v);
	Array(ConstStringT<INode *> v);
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
	virtual const INode *enableMTAccess() const;

	virtual INode *clone(PFactory factory) const;

	virtual bool operator==(const INode &other) const;

	virtual INode *replace(natural name, Value newValue, Value *prevValue = 0);
	virtual INode * clear();


protected:
	FieldList_t list;
};

class TextField: public LeafNode {
public:
	TextField(ConstStrW x):value(x),utf(0) {}
	~TextField();

	virtual NodeType getType() const {return ndString;}
	virtual ConstStrW getString() const {return value;}
	virtual integer getInt() const;
	virtual linteger getLongInt() const;
	virtual double getFloat() const;
	virtual bool getBool() const {return !value.empty();}
	virtual bool isNull() const {return false;}
	ConstStrA getStringUtf8() const;
	virtual const INode * enableMTAccess()const;
	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;

	AutoArray<wchar_t,SmallAlloc<32> > value;
	mutable StringA *utf;
};


class TextFieldA: public LeafNode {
public:
	TextFieldA(ConstStrA x):value(x),wide(0) {}
	~TextFieldA();

	virtual NodeType getType() const {return ndString;}
	virtual integer getInt() const;
	virtual linteger getLongInt() const;
	virtual double getFloat() const;
	virtual bool getBool() const {return !value.empty();}
	virtual bool isNull() const {return false;}
	ConstStrA getStringUtf8() const  {return value;}
	virtual const INode * enableMTAccess() const;
	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;
	virtual bool isUtf8() const {return true;}
	virtual ConstStrW getString() const;


	AutoArray<char,SmallAlloc<32> > value;
	mutable String *wide;
};

class EmptyString: public LeafNode {
public:
	EmptyString() {}

	virtual NodeType getType() const {return ndString;}
	virtual integer getInt() const {return 0;}
	virtual linteger getLongInt() const {return 0;}
	virtual double getFloat() const {return 0;}
	virtual bool getBool() const {return false;}
	virtual bool isNull() const {return false;}
	ConstStrA getStringUtf8() const  {return ConstStrA();}
	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;
	virtual bool isUtf8() const {return true;}
	virtual ConstStrW getString() const {return ConstStrW();}
};

class ZeroNumber: public LeafNode {
public:
	ZeroNumber() {}

	virtual NodeType getType() const {return ndInt;}
	virtual integer getInt() const {return 0;}
	virtual linteger getLongInt() const {return 0;}
	virtual double getFloat() const {return 0;}
	virtual bool getBool() const {return false;}
	virtual bool isNull() const {return false;}
	ConstStrA getStringUtf8() const  {return ConstStrA("0");}
	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;
	virtual bool isUtf8() const {return true;}
	virtual ConstStrW getString() const {return ConstStrW(L"0");}
};

class SingleCharacter: public LeafNode {
public:
	SingleCharacter(char x):x(x) {}

	virtual NodeType getType() const {return ndString;}
	virtual integer getInt() const {return getNum();}
	virtual linteger getLongInt() const {return getNum();}
	virtual double getFloat() const {return getNum();}
	virtual bool getBool() const {return false;}
	virtual bool isNull() const {return false;}
	ConstStrA getStringUtf8() const  {return ConstStrA(x);}
	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;
	virtual bool isUtf8() const {return true;}
	virtual ConstStrW getString() const {return ConstStrW(x);}

	char x;
protected:
	int getNum() const;
};


class LeafNodeConvToStr: public LeafNode {
public:
	LeafNodeConvToStr():txtBackend(0) {}
	~LeafNodeConvToStr();

	TextFieldA &getTextNode() const;

	virtual TextFieldA *createTextNode() const = 0;

protected:
	mutable TextFieldA *txtBackend;
};


class IntField : public LeafNodeConvToStr {
public:
	IntField(integer x):x(x) {}
	virtual NodeType getType() const {return ndInt;}
	virtual ConstStrW getString() const {return getTextNode().getString();}
	virtual ConstStrA getStringUtf8() const {return getTextNode().getStringUtf8();}
	virtual integer getInt() const {return x;}
	virtual linteger getLongInt() const {return x;}
	virtual double getFloat() const {return double(x);}
	virtual bool getBool() const {return x != 0;}
	virtual bool isNull() const {return false;}
	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;

	virtual TextFieldA *createTextNode() const;

	integer x;
	mutable StringA strx;
};

class IntField64 : public LeafNodeConvToStr {
public:
	IntField64(linteger x):x(x) {}
	virtual NodeType getType() const {return ndInt;}
	virtual ConstStrW getString() const {return getTextNode().getString();}
	virtual ConstStrA getStringUtf8() const {return getTextNode().getStringUtf8();}
	virtual integer getInt() const {return (integer)x;}
	virtual linteger getLongInt() const {return x;}
	virtual double getFloat() const {return double(x);}
	virtual bool getBool() const {return x != 0;}
	virtual bool isNull() const {return false;}
	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;

	virtual TextFieldA *createTextNode() const;



	linteger x;
	mutable StringA strx;
};

class FloatField : public LeafNodeConvToStr {
public:
	FloatField(double x):x(x) {}
	virtual NodeType getType() const {return ndFloat;}
	virtual ConstStrW getString() const {return getTextNode().getString();}
	virtual ConstStrA getStringUtf8() const {return getTextNode().getStringUtf8();}
	virtual integer getInt() const {return (integer)x;}
	virtual linteger getLongInt() const {return (linteger)x;}
	virtual double getFloat() const {return x;}
	virtual bool getBool() const {return x != 0;}
	virtual bool isNull() const {return false;}

	virtual TextFieldA *createTextNode() const;


	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;


	double x;
	mutable StringA strx;
};

class Null: public LeafNode {
public:
	virtual NodeType getType() const {return ndNull;}
	virtual integer getInt() const {return integerNull;}
	virtual linteger getLongInt() const {return lintegerNull;}
	virtual natural getUInt() const {return naturalNull;}
	virtual lnatural getLongUInt() const {return lnaturalNull;}
	virtual double getFloat() const {return 0;}
	virtual bool getBool() const {return false;}
	virtual bool isNull() const {return true;}

	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;

	static Null *getNull();

};
class Delete: public LeafNode {
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

	static Delete *getDelete();

};



class Bool: public LeafNode {
public:
	Bool(bool b):b(b) {}

	virtual NodeType getType() const {return ndBool;}
	virtual ConstStrA getStringUtf8() const {return b?"true":"false";}
	virtual ConstStrW getString() const { return b ? L"true" : L"false"; }
	virtual integer getInt() const {return b?naturalNull:0;}
	virtual linteger getLongInt() const {return b?naturalNull:0;}
	virtual double getFloat() const {return b?-1:0;}
	virtual bool getBool() const {return b;}
	virtual bool isNull() const {return false;}

	virtual INode *clone(PFactory factory) const;
	virtual bool operator==(const INode &other) const;

	bool b;

	static Bool *getTrue();
	static Bool *getFalse();
};


template<typename T>
class DynNode: public T, public DynObject {
public:
	DynNode() {}

	template<typename X>
	DynNode(const X &x):T(x) {}
};

template<template<typename> class T = DynNode>
class FactoryAlloc: public Factory {
public:

	FactoryAlloc(IRuntimeAlloc  &alloc):alloc(alloc) {}

	virtual PNode createObject() {return new(alloc) T<Object>;}
	virtual PNode createNumber(natural v) {return new(alloc) T<IntField>(v);}
	virtual PNode createNumber(integer v) {return new(alloc) T<IntField>(v);}
#ifdef LIGHTSPEED_HAS_LONG_TYPES
	virtual PNode createNumber(lnatural v) {return new(alloc) T<IntField64>(v);}
	virtual PNode createNumber(linteger v) {return new(alloc) T<IntField64>(v);}
#endif
	virtual PNode createNumber(float v) {return new(alloc) T<FloatField>(v);}
	virtual PNode createNumber(double v) {return new(alloc) T<FloatField>(v);}
	virtual PNode createNumber(bool v) {return new(alloc) T<Bool>(v);}
	virtual PNode createString(ConstStrW v) {return new(alloc) T<TextField>(v);}
	virtual PNode createString(ConstStrA v) {return new(alloc) T<TextFieldA>(v);}
	virtual IRuntimeAlloc *getAllocator() const {return &alloc;}
	virtual Value createArray(ConstStringT<JSON::Value> v) {return new(alloc) T<Array>(v);}
	virtual Value createArray(ConstStringT<JSON::INode *> v) {return new(alloc) T<Array>(v);}


	virtual IFactory *clone() {return new FactoryAlloc(alloc);}

protected:

	IRuntimeAlloc &alloc;
};


}
}



#endif /* LIGHTSPEED_UTILS_JSONIMPL_H_ */
