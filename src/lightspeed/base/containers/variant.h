/*
 * variant.h
 *
 *  Created on: Jul 20, 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_CONTAINERS_VARIANT_H_
#define LIGHTSPEED_BASE_CONTAINERS_VARIANT_H_

#include "../memory/dynobject.h"
#include "../memory/runtimeAlloc.h"
#include "../memory/refcntifc.h"

namespace LightSpeed {

///Class which is able to hold value of any type
class Variant {



public:

	Variant();

	template<typename T>
	Variant(const T &val);

	template<typename T>
	Variant(const T &val, IRuntimeAlloc &alloc);

	template<typename T>
	static Variant ref(const T &value);

	template<typename T>
	static Variant ref(const T &value, IRuntimeAlloc &alloc);

	const std::type_info &getType() const;

	template<typename T>
	bool isType() const;

	template<typename T>
	bool canConvertTo() const;

	template<typename T>
	const T &getValue() const;

	template<typename T>
	const T *getPtr() const;

	bool isNull() const;

	Variant isolate() const;

	Variant getMT() const;

	bool operator==(NullType) const;

	bool operator!=(NullType) const;

	bool operator==(const Variant &v) const;

	bool operator!=(const Variant &v) const;

	template<typename T>
	bool operator==(const T &v) const;

	template<typename T>
	bool operator!=(const T &v) const;

	void throwIt() const;

protected:

	class IData: public DynObject, public RefCntObj {
	public:
		virtual ~IData() {}
		virtual const void *getContent() const = 0;
		virtual const std::type_info &getType() const = 0;
		virtual void throwPtr() const = 0;
		virtual void throwValue() const = 0;
		virtual IData *clone() const = 0;
		virtual bool same(const IData *other) const = 0;
	};

	typedef RefCntPtr<IData> PData;

	template<typename T>
	class DataImpl: public IData {
	public:
		DataImpl(const T &val):val(val) {}
		virtual const void *getContent() const {return &val;}
		virtual const std::type_info &getType() const {return typeid(val);}
		virtual void throwPtr() const {throw &val;}
		virtual void throwValue() const {throw val;}
		virtual IData *clone() const {return new DataImpl<T>(val);}
		virtual bool same(const IData *other) const {
			return other != 0 && other->getType() == DataImpl::getType() &&
					val == *reinterpret_cast<const T *>(other->getContent());
		}

	protected:
		T val;
	};


	PData dataPtr;

	Variant(const PData &data):dataPtr(data) {}

};

template<typename T>
inline Variant::Variant(const T& val) :dataPtr(new DataImpl<T>(val)) {}

template<typename T>
inline Variant::Variant(const T& val, IRuntimeAlloc& alloc):dataPtr(new(alloc) DataImpl<T>(val)) {}

template<typename T>
inline Variant Variant::ref(const T& value) {
	return Variant(new DataImpl<const T &>(value));
}

template<typename T>
inline Variant Variant::ref(const T& value, IRuntimeAlloc& alloc) {
	return Variant(new(alloc) DataImpl<const T &>(value));
}

template<typename T>
inline bool Variant::isType() const {
	return getType() == typeid(T);
}

template<typename T>
inline bool Variant::canConvertTo() const {
	return getPtr<T>() != 0;
}

template<typename T>
inline const T& Variant::getValue() const {
	if (isType<T>()) return *reinterpret_cast<const T *>(dataPtr->getContent());
	else throw std::bad_cast();
}

template<typename T>
inline const T* Variant::getPtr() const {
	try {
		if (isType<T>()) return reinterpret_cast<const T *>(dataPtr->getContent());
		dataPtr->throwPtr();
	} catch (const T *val) {
		return val;
	} catch (...) {
		return 0;
	}
}

template<typename T>
inline bool Variant::operator ==(const T& v) const {
	return isType<T>() && *reinterpret_cast<const T *>(dataPtr->getContent()) == v;
}

template<typename T>
inline bool Variant::operator !=(const T& v) const {
	return !operator==(v);
}


}

#endif /* LIGHTSPEED_BASE_CONTAINERS_VARIANT_H_ */
