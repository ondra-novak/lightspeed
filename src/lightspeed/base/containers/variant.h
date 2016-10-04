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

	///Construct empty variant
	Variant();

	///Construct Variant object which carries a value of given type
	/***
	 * @param val value tostore there
	 */
	template<typename T>
	Variant(const T &val);


	///Construct Variant object which carries a value of given type. You can specify allocator
	/***
	 * @param val value tostore there
	 * @param alloc allocator to allocate internal structure
	 */
	template<typename T>
	Variant(const T &val, IRuntimeAlloc &alloc);

	///Constructs Variant, which carries reference to given value
	/**
	 * It has same meaning as reference operator. Variant stores the reference, original variable must
	 * kept alive.
	 * @param value reference to the variable
	 * @return variant object
	 */
	template<typename T>
	static Variant ref(const T &value);

	///Constructs Variant, which carries reference to given value
	/**
	 * It has same meaning as reference operator. Variant stores the reference, original variable must
	 * kept alive.
	 * @param value reference to the variable
	 * @param alloc allocator used to allocate internal structures
	 * @return variant object
	 */
	template<typename T>
	static Variant ref(const T &value, IRuntimeAlloc &alloc);

	///Retrieves type of value stored there
	/**
	 * @return type information about the value
	 */
	const std::type_info &getType() const;

	///Determines, whether value is given type
	/**
	 * @tparam T the type we asking
	 * @retval true variant contains that type
	 * @retval false variant doesn't contains that type
	 */
	template<typename T>
	bool isType() const;

	///Determines, whether value can be converted to other type
	/**
	 * Conversion is only possible from descendant to base type. It will fail, if there is
	 * multiple non-virtual bases. Conversion is performed in runtime based on RTTI.
	 * Function uses exceptions to perfrom conversions.
	 * This is the reason, why function can be slow and not woringing everytime.
	 *
	 * @tparam T type we asking
	 * @retval true convertion is possible
	 * @retval false convertion is not possible
	 */
	template<typename T>
	bool canConvertTo() const;

	///Retrieve value if type matches
	/**
	 * @tparam T type we asking
	 * @return reference to value
	 * @exception std::bad_cast type mismatch
	 */
	template<typename T>
	const T &getValue() const;

	///Retrieve pointer to the value if type matches
	/**
	 * @tparam T type we asking
	 * @return pointer to the value. Function returns nullptr for type mismatch
	 */
	template<typename T>
	const T *getPtr() const throw();

	///Determines, whether object is NULL
	bool isNull() const;

	///Ensures, that value is not shared
	/** Variant object can be copied. Copying is done by sharing one instance accros all copies. This
	 * isn't problem because value stored in the Variant object is considered as constant. It should
	 * not change, so from the outside view, every copy should look same. However, this sharing is
	 * always helpful. Function isolate eliminates sharing for current instance making copy of original
	 * object.
	 * @return isolated copy of object
	 */
	Variant isolate() const;

	///Determines whether object is NULL
	bool operator==(NullType) const;
	///Determines whether object is not NULL
	bool operator!=(NullType) const;

	///Compares two objects
	/**
	 * @param v other object
	 * @retval true objects are same (including types)
	 * @retval false objects are not same (may be same type, but different value)
	 */
	bool operator==(const Variant &v) const;

	///Compares two objects
	/**
	 * @param v other object
	 * @retval false objects are same (including types)
	 * @retval true objects are not same (may be same type, but different value)
	 */
	bool operator!=(const Variant &v) const;

	///Compares two objects
	/**
	 * @param v other object
	 * @retval true objects are same (including types)
	 * @retval false objects are not same (may be same type, but different value)
	 */
	template<typename T>
	bool operator==(const T &v) const;

	///Compares two objects
	/**
	 * @param v other object
	 * @retval false objects are same (including types)
	 * @retval true objects are not same (may be same type, but different value)
	 */
	template<typename T>
	bool operator!=(const T &v) const;

	///Throw content of object as exception (as value)
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
inline const T* Variant::getPtr() const throw() {
	try {
		if (isType<T>()) return reinterpret_cast<const T *>(dataPtr->getContent());
		dataPtr->throwPtr();
		throw;
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
