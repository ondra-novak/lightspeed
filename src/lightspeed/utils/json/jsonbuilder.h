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
	CObject operator()(ConstStrA name, const ConstValue &value) const;
	CObject operator()(ConstStrA name, const Container &value) const;
	///Creates empty object
	Value operator()(NullType) const;

	///Creates container object.
	/** Container object enables to add ConstValue without copying it. However result is Container, not Value */
	template<typename T>
	CObject container(ConstStrA name, const T &value) const;
	CObject container(const ConstValue &value) const;



	template<typename T>
	Value operator()(const T &val) const;
	Container operator()(const ConstStringT<Container> &val) const;
	Container operator()(const ConstStringT<ConstValue> &val) const;

	template<typename T>
	Array operator,(const T &x) const {return operator<<(x);}
	CArray operator,(const Container &x) const {return operator<<(x);}
	CArray operator,(const ConstValue &x) const {return operator<<(x);}
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
	CArray operator<<(const Container &x) const;
	CArray operator<<(const ConstValue &x) const;


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
		Object &operator()(ConstStrA name, const T &value) {
			set(name,factory->newValue(value));
			return *this;
		}
		///creates or access the subobject of given name
		/**
		 * @code
		 * (obj/"aaa"/"bbb")("xxx",true)("yyy",100) -> {"aaa":{"bbb":{"xxx":true, "yyy":100}}}
		 * @param name name of key to create subobject
		 * @return subobject
		 */
		Object operator/(ConstStrA name);
		Object &operator()(ConstStrA name, NullType n) {set(name, factory->newValue(n));return *this;};
		///Converts Object to Container Object
		/** This allows to add ConstValue object to the container, however, result will be Container */
		CObject container() const {return CObject(factory,*this);}
	protected:
	};

	class CObject: public CCommon {
	public:

		CObject(const PFactory &factory, const Container &nd):CCommon(factory,nd) {}
		CObject(const PFactory &factory):CCommon(factory,factory->newObject()) {}

		template<typename T>
		CObject &operator()(ConstStrA name, const T &value) {
			setValue(name,value,&value);
			return *this;
		}
		CObject &operator()(ConstStrA name, NullType n) {set(name, n);return *this;};
	protected:
		template<typename T>
		void setValue(ConstStrA name, const T &value, ...) {
			set(name, factory->newValue(value));
		}
		template<typename T>
		void setValue(ConstStrA name, const T &value, const ConstValue *) {
			set(name, value);
		}
	};

	class Array: public Common {
	public:

		Array(const PFactory &factory, const Value &nd):Common(factory,nd) {}
		Array(const PFactory &factory):Common(factory,factory->newArray()) {}

		template<typename T>
		Array &operator,(const T &x) {return operator<<(x);}
		template<typename T>
		Array &operator<<(const T &x) {add(factory->newValue(x));return *this;}
		Array &operator<<(NullType n) {add(factory->newValue(n)); return *this;}
		/** This allows to add ConstValue object to the container, however, result will be Container */
		CArray container() const {return CArray(factory,*this);}
	protected:

	};

	class CArray: public CCommon {
	public:

		CArray(const PFactory &factory, const Container &nd):CCommon(factory,nd) {}
		CArray(const PFactory &factory):CCommon(factory,factory->newArray()) {}

		template<typename T>
		CArray &operator,(const T &x) {return operator<<(x);}
		template<typename T>
		CArray &operator<<(const T &x) {setValue(x,&x); return *this;}
		CArray &operator<<(NullType n) {add(factory->newValue(n)); return *this;}
	protected:
		template<typename T>
		void setValue(const T &value, ...) {
			add(factory->newValue(value));
		}
		template<typename T>
		void setValue(const T &value, const ConstValue *) {
			add(value);
		}

	};


	public:
		PFactory factory;




protected:
};


template<typename T>
inline Builder::Object Builder::operator ()(ConstStrA name, const T& value) const {
	Object nd(factory);
	nd(name, value);
	return nd;
}

template<typename T>
inline Builder::CObject Builder::container(ConstStrA name, const T& value) const {
	CObject nd(factory);
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
Value Builder::operator()(const T &val) const {
	return factory->newValue(val);
}

}
}

#endif /* LIGHTSPEED_UTILS_JSON_JSONBUILDER_H_ */


