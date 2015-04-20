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


	///Creates new object with initial value
	/**
	 * @param name key name
	 * @param value value
	 * @return object builder, it carries building node and can be used to append additional values
	 */
	template<typename T>
	Object operator()(ConstStrA name, const T &value) const;
	///Creates new object with initial value which is another object or array
	/**
	 * @param name key name
	 * @param value value as PNode
	 * @return object builder, it carries building node and can be used to append additional values
	 */
	Object operator()(ConstStrA name, PNode nd) const;
	///Attachs builder to existing object
	/**
	 * @param nd object stored as node. Function creates builder, so you are able to append new values
	 * using the builder
	 * @return new object nuilder
	 */
	Object operator()(PNode nd) const;
	///Creates empty object
	Object operator()() const;
	///Creates empty object
	Object operator()(Empty_t) const;
	///Creates empty object
	PNode operator()(NullType) const;

	template<typename T>
	PNode operator()(const T &val) const;

	///Creates a new array and enters first value into it
	/**
	 * @param value put into array
	 * @return array builder
	 */
	template<typename T>
	Array operator[](const T &) const;
	template<typename T>
	Array operator,(const T &x) const {return operator[](x);}
	///Attachs builder to existing array
	/**
	 * @param nd array stored as node. Function creates builder, so you are able to append new values
	 * using the builder
	 * @return new object nuilder
	 *
	 * @note There is no function to create new array with appended PNode as Object supports. If you
	 * with to do such operation, simply create empty array and append the PNode.
	 */
	Array operator[](PNode nd) const;
	///Creates empty array
	Array operator[](Empty_t) const;

	Array array() const;

	Object object() const;

	class Common: public PNode {
	public:
		PFactory factory;

		Common (PFactory factory, PNode nd):PNode(nd), factory(factory) {}

	};

	class Object: public Common {
	public:

		Object(PFactory factory, PNode nd):Common(factory,nd) {}
		Object(PFactory factory):Common(factory,factory->newObject()) {}

		template<typename T>
		Object operator()(ConstStrA name, const T &value);
		Object operator()(ConstStrA name, PNode nd);
	protected:
		template<typename T>
		void append(ConstStrA name, const T &item, MTrue ) {(*this)->add(name, item);}
		template<typename T>
		void append(ConstStrA name, const T &item, MFalse ) {(*this)->add(name, factory->newValue(item));}

	};

	class Array: public Common {
	public:

		Array(PFactory factory, PNode nd):Common(factory,nd) {}
		Array(PFactory factory):Common(factory,factory->newArray()) {}

		template<typename T>
		Array operator[](const T &);
		template<typename T>
		Array operator,(const T &x) {return operator[](x);}
		Array operator[](PNode nd);
	protected:
		template<typename T>
		void append(const T &item, MTrue ) {(*this)->add(item);}
		template<typename T>
		void append(const T &item, MFalse ) {(*this)->add(factory->newValue(item));}

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
inline Builder::Array Builder::operator [](const T&value) const {
	Array nd(factory);
	nd[value];
	return nd;
}

template<typename T>
inline Builder::Object Builder::Object::operator ()(ConstStrA name, const T& value) {
	append(name, value,typename MIsConvertible<T,const PNode &>::MValue());
	return *this;
}

template<typename T>
inline Builder::Array Builder::Array::operator [](const T& value) {
	append(value,typename MIsConvertible<T,const PNode &>::MValue());
	return *this;
}

}
}

#endif /* LIGHTSPEED_UTILS_JSON_JSONBUILDER_H_ */


