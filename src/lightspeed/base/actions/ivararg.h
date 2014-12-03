/*
 * ivararg.h
 *
 *  Created on: 10.8.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ACTIONS_IVARARG_H_
#define LIGHTSPEED_ACTIONS_IVARARG_H_

#include "../typeinfo.h"
#include "../memory/runtimeAlloc.h"
#include "../exceptions/badcast.h"
#include "../memory/stdAlloc.h"

namespace LightSpeed {

template<typename T>
class VarArgField;

///Interface used to enumerate through arguments
class IVarArgEnum {
public:

	///information about the argument
	struct ArgInfo {
		///pointer to the argument in memory
		const void *ptr;
		///size of argument in bytes
		natural size;
		///runtime type information
		TypeInfo type;

		///ctor
		ArgInfo(const void *ptr,natural size,TypeInfo type)
			:ptr(ptr),size(size),type(type) {}
	};


	///called for every argument
	/**
	 * @param arg information about the argument
	 * @retval true stop enumeration
	 * @retval false continue enumeration
	 */
	virtual bool operator()(const ArgInfo &arg) const = 0;
	virtual ~IVarArgEnum() {}
};

///Allowing access VarArg using common interface without need to use templates
/**
 * Note that interface can only limited features in compare to generic
 * VarArg template objects. That because, all type informations are
 * retrieved at runtime, so compiler cannot check and manage type specific
 * code, because has no idea about objects behind this interface.
 *
 * Main advantage is, that  IVarArg can be used with virtual functions,
 * which cannot be declared as template. Disadvantage is slightly worse
 * performance while accessing the data of VarArg container
 *
 * Please note, that object behind is constructed without copying data, all
 * variable stored as reference. If you wish to make copy with data, use
 * copy() function
 *
 */
class IVarArg {
public:

	///Declaration of type containing informations about argument
	typedef IVarArgEnum::ArgInfo ArgInfo;

	///creates copy of container as it is
	/**
	 * @param alloc allocator used to allocate object. Its reference is
	 * stored with the object and so returned pointer can be deallocated
	 * using standard delete;
	 * @return pointer to allocated object. It have to be destroyed using
	 * standard @b delete when no longer needed.
	 */
	virtual IVarArg *clone(IRuntimeAlloc &alloc) const = 0;


	///creates copy of the container including data
	/**
	 * Function clone() always create container holding references to the data
	 * of original object. This function also copies data. There is no difference
	 * seen outside, but function copy() can be slightly slower
	 * @param alloc allocator use to allocate memory
	 * @return pointer to allocated object. It have to be destroyed using
	 * standard @b delete when no longer needed
	 */
	virtual IVarArg *copy(IRuntimeAlloc &alloc) const = 0;

	///Retrieves informations about single argument
/**
	 * @param index zero-base index of the argument
	 * @return informations about the argument
	 */
	virtual ArgInfo getArg(natural index) const = 0;

	///Throws pointer to argument as the exception
	/**
	 * This is the only legal way, how to perform conversion from
	 * inherited to base object when dynamic_cast cannot be used. Argument
	 * is thrown as const T *, while T is original type of the argument.
	 *
	 * It can be caught as const U *, where U is base of the object. If
	 * exception is not caught, argument is not convertible.
	 *
	 * @param index of argument
	 *
	 * @note Throwing exception is complex operation, and can degrade
	 * performance, if used often.
	 */
	virtual void throwArg(natural index) const = 0;

	///Returns count of arguments
	/**
	 * @return count of arguments
	 */
	virtual natural length() const = 0;


	///Processes all arguments
	/**
	 * @param fn functor called for every argument
	 * @retval true enumeration has been stopped
	 * @retval false enumeration finished all arguments
	 */
	virtual bool forEach(const IVarArgEnum &fn) const = 0;

	///Retrieves value of argument at position
	/**
	 * @tparam expected type of argument. Function supports conversion
	 * from inherited to base type
	 * @param index index of the argument
	 * @return value of the argument
	 * @exception BadCastException argument cannot be converted
	 */
	template<typename T>
	const T &getValue(natural index) const {
		ArgInfo nfo = getArg(index);
		if (typeid(T) == nfo.type) return *reinterpret_cast<const T *>(nfo.ptr);
		try {
			throwArg(index);
			throw;
		} catch (const T *e) {
			return *e;
		} catch (...) {
			throw BadCastException(THISLOCATION,nfo.type,typeid(T));
		}
	}

	template<typename K, typename VV>
	VV field(const K fieldName, const VV &defVal) const {
		bool found ;
		return field(fieldName,defVal,found);
	}

	template<typename K, typename VV>
	VV field(const K fieldName, const VV &defVal, bool &ok) const {

		class FindField: public IVarArgEnum {
		public:

			FindField(natural &pos, const K &fieldName):pos(pos),fieldName(fieldName) {}

			virtual bool operator()(const ArgInfo &arg) const {
				if (arg.type == TypeInfo(typeid(VarArgField<K>))) {
					const VarArgField<K> *varg = reinterpret_cast<const VarArgField<K> *>(arg.ptr);
					if (*varg == fieldName) return true;
				}
				pos++;
				return false;
			}

		protected:
			 natural &pos;
			const K &fieldName;


		};

		natural pos = 0;
		if (forEach(FindField(pos,fieldName))) {
			ok = true;
			return getValue<VV>(pos+1);
		} else {
			ok = false;
			return defVal;
		}

	}

	///destructor
	virtual ~IVarArg() {}
};



}  // namespace LightSpeed

#endif /* LIGHTSPEED_ACTIONS_IVARARG_H_ */
