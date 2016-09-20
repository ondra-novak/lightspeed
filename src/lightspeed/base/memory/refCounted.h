/*
 * refCounter.h
 *
 *  Created on: 20. 9. 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_MEMORY_REFCOUNTED_H_
#define LIGHTSPEED_BASE_MEMORY_REFCOUNTED_H_

#include "../../mt/atomic_type.h"
#include "pointer.h"

namespace LightSpeed {

template<typename T> class RefCountedPtr;


class RefCounted {
public:
	RefCounted();

	///adds reference
	/** Adds reference to the object. Note that reference can be invisible in other threads until
	 *  it is committed. This function is used by RefCountedPtr
	 */
	void addRef() const {updateCounter(1);}
	///adds reference and commit it to the object
	/**
	 * Adds reference and commits reference state to the object. Function is slow, because commit operation
	 * requires interlocked operation
	 */
	void addRefCommit() const {updateCounter(1);commitRef();}
	///Releases reference
	/** Removes one reference from the object. Note that removal of the reference can be invisible in
	 * other threads until it is committed. This function is used by RefCountedPtr.
	 *
	 * @note removing reference doesn't mean that object will be released immediately even if there are
	 *  no more references. This because current thread still can still keep reference in local table.
	 *  This is because the current thread doesn't know anything about references in other thread without
	 *  accessing the object's counter - which is very expensive operation
	 */
	void release() const {updateCounter(-1);}
	///Releases reference and commit changes to the object
	/** Removes one reference. It also commits state to the object. If there are no more references, object
	 * is destroyed. Note that function is slow because the function requires interlocked operation (which is
	 * very expensive in multicore environment)
	 *
	 * @note function is called by function RefCountedPtr::clearAndCommit()
	 */
	void releaseCommit() const {updateCounter(-1);commitRef();}
	///Commits cached reference for current thread
	/** @note function only commits for current thread. It cannot commit reference in other threads */
	void commitRef() const;

	///Determines, whether object has more references
	/**
	 * @see RefCountedPtr::isShared
	 *
	 */
	bool isShared() const;

	virtual ~RefCounted();

	///Commit all references
	/**
	 * Commits all cached referenced for current thread. You need to call this function
	 * if you need to ensure, that there is no living object which has been left before.
	 * Note that function commits references only for current thread. It cannot work
	 * with other threads (you have to call it in each thread if you need it)
	 *
	 * Function is called before each thread exits.
	 *
	 */
	static void commitAllRefs() throw();

private:
	mutable atomic counter;

	class PointerCacheItem;
	class PointerCache;

	void updateCounter(atomicValue diff) const;
	void updateCounterMt(atomicValue diff) const;

	template<typename T> friend class RefCountedPtr;

};


template<typename T> class RefCountedPtr: public Pointer<T> {
public:

	typedef Pointer<T> Super;
    ///Constructs empty pointer
	RefCountedPtr() {}


    ///Constructs empty pointer
	RefCountedPtr(NullType x):Super(x) {}



    ///Constructs a pointer and add reference
    /**
     * @para p pointer to assign the new instance
     */
	RefCountedPtr(T *p):Super(p) {
        addRefPtr();
    }

   ///Conversion from wrapped pointer
   /**
    * @param p
    *
    * @note GCC requieres const-reference here. Clang will compile without it.
    */
	RefCountedPtr(const Pointer<T> &p):Super(p) {
    	addRefPtr();
    }
    ///Copy constructor
    /**@param other source object
     */
	RefCountedPtr(const RefCountedPtr &other)
        :Super(other.get()) {
        addRefPtr();
    }



    ///Destroy instance and release reference
    /**
     * Destroys instance and releases reference. Object can be
     * released, if reference count reaches zero
     */
    ~RefCountedPtr() {
        releasePtr();
    }



    operator RefCountedPtr<const T>() const {
    	return RefCountedPtr<const T>(this->ptr);
    }

    ///Assignment
    /**@param other source object
     * @return this
     */
    RefCountedPtr &operator=(const RefCountedPtr &other) {
    	other.addRefPtr();
    	releasePtr();
    	Super::operator=(other);
        return *this;
    }

	//C++x11
#ifdef LIGHTSPEED_ENABLE_CPP11

    RefCountedPtr(RefCountedPtr &&other) {
		this->ptr = other.ptr;
		other.ptr = 0;
	}
    RefCountedPtr &operator=(RefCountedPtr &&other) {
		if (this != &other) {
			releasePtr();
			this->ptr = other.ptr;
			other.ptr = 0;
		}
		return *this;
	}

#endif

	///True, if object is shared at more than once
	/**
	 * @retval true object is shared
	 * @retval false object is not shared, this is only reference
	 *
	 * @note Function works well unless the pointer is shared with other threads. Once
	 * the pointer is shared by more threads, each thread can keep a reference even when
	 * there is living reference in that thread (reference is kept in local cache). Because
	 * of this, only the response 'false' is accurate. The response 'true' can be inaccurate.
	 */
	bool isShared() const {
		if (this->ptr == 0) return false;
		return this->isShared();
	}


	void clear() {
		releasePtr();
	}


	void clearAndCommit() {
		if (this->ptr)
			this->ptr->releaseCommit();
	}

protected:


    inline void addRefPtr() const {
        if (this->ptr) {
        	this->ptr->updateCounter(1);
        }
    }
    inline void releasePtr() const {
        if (this->ptr) {
        	this->ptr->updateCounter(-1);
        }
    }


};



}




#endif /* LIGHTSPEED_BASE_MEMORY_REFCOUNTED_H_ */
