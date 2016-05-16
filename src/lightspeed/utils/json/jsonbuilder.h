/*
 * jsonbuilder.h
 *
 *  Created on: Sep 5, 2014
 *      Author: ondra
 */

#ifndef LIGHTSPEED_UTILS_JSON_JSONBUILDER_H_
#define LIGHTSPEED_UTILS_JSON_JSONBUILDER_H_

#include "json.h"
#include "../../base/meta/metaIf.h"

namespace LightSpeed {namespace JSON {

///New JSON builder - better way to create JSON object
/** old builders are obsolete from now
 *
 * Builder has minimum set of member functions. Everything is done by operators
 *
 * Once builder is created, it is not determined, whether it is object or array. It is determined
 * by first operation
 *
 * @code
 * Builder jsondata;
 * jsondata("foo",10)("bar",true) ;// create object
 * @endcode
 *
 * @code
 * Builder jsondata;
 * jsondata[10][20]["Test"]; //create array
 * @endcode
 *
 * To create nested objects, use operator + to create new builder
 *
 * @code
 * Builder jsondata;
 * jsondata("outer",jsondata("inner",10)("inner2",20))("outer2",false)
 * @endcode
 *
 *
 *
 * */
class Builder {
public:

	Builder();
	Builder(PFactory factory);
	Builder(IRuntimeAlloc &alloc);


	class Common;
	class Object;
	class Array;
	class CObject;
	class CArray;


	///Creates new object with initial value
	/**
	 * @param name key name
	 * @param value value
	 * @return object builder, it carries building node and can be used to append additional values
	 */
	template<typename T>
	Object operator()(ConstStrA name, const T &value) const;
	///Creates empty object
	Value operator()(NullType) const;



	template<typename T>
	Value operator()(const T &val) const;
	Container operator()(const ConstStringT<Container> &val) const;
	Container operator()(const ConstStringT<ConstValue> &val) const;

	template<typename T>
	Array operator,(const T &x) const {return operator<<(x);}
	///use serialization operator to construct array or feed array with values
	/**
	 * @code
	 * json << 10,20,30
	 * json << 10 << 20 << 30
	 * json.array(x) << 10,20,30
	 * @param x
	 * @return
	 */
	template<typename T>
	Array operator<<(const T &x) const;
	///Attachs builder to existing array
	/**
	 * @param nd array stored as node. Function creates builder, so you are able to append new values
	 * using the builder
	 * @return new object nuilder
	 *
	 * @note There is no function to create new array with appended PNode as Object supports. If you
	 * with to do such operation, simply create empty array and append the PNode.
	 */
	Array array(const Value &nd) const;
	Object object(const Value &nd) const;
	CArray array(const Container &nd) const;
	CObject object(const Container &nd) const;
	CArray array(const ConstValue &nd) const;
	CObject object(const ConstValue &nd) const;
	///Creates empty array
	Array operator[](Empty_t) const;

	Array array() const;

	Object object() const;

	class CCommon: public Container {
	public:
		PFactory factory;

		CCommon (const PFactory &factory, const Container &nd):Container(nd), factory(factory) {}

	};

	class Common: public Value {
	public:
		PFactory factory;

		Common (const PFactory &factory, const Value &nd):Value(nd), factory(factory) {}

	};

	class Object: public Common {
	public:

		Object(const PFactory &factory, const Value &nd):Common(factory,nd) {}
		Object(const PFactory &factory):Common(factory,factory->newObject()) {}

		template<typename T>
		Object &operator()(ConstStrA name, const T &value);
		Object &operator()(ConstStrA name, NullType n) {set(name, n, MFalse());return *this;};
		CObject operator()(ConstStrA name, const ConstValue &n);
	protected:
		template<typename T>
		void set(ConstStrA name, const T &item, MTrue ) {Common::set(name, item);}
		template<typename T>
		void set(ConstStrA name, const T &item, MFalse ) {Common::set(name, factory->newValue(item));}
	};

	class CObject: public CCommon {
	public:

		CObject(const PFactory &factory, const Container &nd):CCommon(factory,nd) {}

		template<typename T>
		CObject &operator()(ConstStrA name, const T &value);
		CObject &operator()(ConstStrA name, NullType n) {set(name, n, MFalse());return *this;};
	protected:
		template<typename T>
		void set(ConstStrA name, const T &item, MTrue ) {CCommon::set(name, item);}
		template<typename T>
		void set(ConstStrA name, const T &item, MFalse ) {CCommon::set(name, factory->newValue(item));}
	};

	class Array: public Common {
	public:

		Array(const PFactory &factory, const Value &nd):Common(factory,nd) {}
		Array(const PFactory &factory):Common(factory,factory->newArray()) {}

		template<typename T>
		Array &operator,(const T &x) {return operator<<(x);}
		template<typename T>
		Array &operator<<(const T &x);
		Array &operator<<(NullType n) {append(n,MFalse()); return *this;}
		CArray operator<<(const ConstValue &x);
	protected:
		template<typename T>
		void append(const T &item, MTrue ) {Common::add(item);}
		template<typename T>
		void append(const T &item, MFalse ) {Common::add(factory->newValue(item));}

	};

	class CArray: public CCommon {
	public:

		CArray(const PFactory &factory, const Container &nd):CCommon(factory,nd) {}

		template<typename T>
		CArray &operator,(const T &x) {return operator<<(x);}
		template<typename T>
		CArray &operator<<(const T &x);
		CArray &operator<<(NullType n) {append(n,MFalse()); return *this;}
	protected:
		template<typename T>
		void append(const T &item, MTrue ) {CCommon::add(item);}
		template<typename T>
		void append(const T &item, MFalse ) {CCommon::add(factory->newValue(item));}

	};

	public:
		PFactory factory;

protected:
	template<typename T>
	JSON::PNode convVal(const T &t, MTrue) const { return t; }
	template<typename T>
	JSON::PNode convVal(const T &t, MFalse) const { return factory->newValue(t); }
};

template<typename T>
JSON::PNode LightSpeed::JSON::Builder::operator()(const T &val) const
{
	return convVal(val, typename MIsConvertible<T, PNode>::MValue());
}


template<typename T>
inline Builder::Object Builder::operator ()(ConstStrA name, const T& value) const {
	Object nd(factory);
	nd(name, value);
	return nd;
}

template<typename T>
inline Builder::Array Builder::operator <<(const T&value) const {
	Array nd(factory);
	nd<<value;
	return nd;
}

template<typename T>
inline Builder::Object &Builder::Object::operator ()(ConstStrA name, const T& value) {
	set(name, value,typename MIsConvertible<T,const PNode &>::MValue());
	return *this;
}
template<typename T>
inline Builder::CObject &Builder::CObject::operator ()(ConstStrA name, const T& value) {
	set(name, value,typename MIsConvertible<T,const PNode &>::MValue());
	return *this;
}

template<typename T>
inline Builder::Array &Builder::Array::operator <<(const T& value) {
	append(value,typename MIsConvertible<T,const PNode &>::MValue());
	return *this;
}

template<typename T>
inline Builder::CArray &Builder::CArray::operator <<(const T& value) {
	append(value,typename MIsConvertible<T,const PNode &>::MValue());
	return *this;
}

}
}

#endif /* LIGHTSPEED_UTILS_JSON_JSONBUILDER_H_ */


