/*
 * refCounted.h
 *
 *  Created on: Jan 4, 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_MEMORY_REFCOUNTED_H_
#define LIGHTSPEED_BASE_MEMORY_REFCOUNTED_H_
#include "../../mt/atomic_type.h"
#include "pointer.h"

namespace LightSpeed {


///Faster and still multithread safe implementation of reference-counted objects
class RefCounted {
public:
	RefCounted();


	void addRef() const;
	void release() const;

	///All object must have virtual destructors to handle correctly deletion
	virtual ~RefCounted() {}
protected:
	mutable atomic refCounter;
	static const atomicValue threadRef;

	friend class PointerCache;
};

template<typename T>
class RefCountedPtr: public Pointer<T> {
public:

	RefCountedPtr() {}
	RefCountedPtr(T *ptr):Pointer<T>(ptr) {
		if (ptr) ptr->addRef();
	}
	RefCountedPtr(const RefCountedPtr &other):Pointer<T>(other.ptr) {
		if (other.ptr) other.ptr->addRef();
	}
	RefCountedPtr(const Pointer<T> &other):Pointer<T>(other.ptr) {
		if (other.ptr) other.ptr->addRef();
	}

	RefCountedPtr &operator=(const RefCountedPtr &other) {
		if (other.ptr != this->ptr) {
			other.ptr->addRef();
			try {
				this->ptr->release();
				this->ptr = other.ptr;
			} catch (...) {
				other.ptr->release();
				throw;
			}
		}
		return *this;
	}

};


} /* namespace LightSpeed */

#endif /* LIGHTSPEED_BASE_MEMORY_REFCOUNTED_H_ */
