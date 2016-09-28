/*
 * weakref.h
 *
 *  Created on: 28. 9. 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_MEMORY_WEAKREF_H_
#define LIGHTSPEED_BASE_MEMORY_WEAKREF_H_


#include "../../mt/atomic.h"
#include "../../mt/threadMinimal.h"
#include "../memory/refCntPtr.h"
#include "dynobject.h"

namespace LightSpeed {

class ISleepingObject;

///Retrieves allocator for internal objects of the WeakRef
/** It returns allocator for Promises. Currently this allocator is shared. */
IRuntimeAlloc &getWeakRefAllocator();

template<typename T> class WeakRefPtr;

///Implements weak reference
/** Class WeakRef<T> is primarly designed to correcly handle pointer invalidation
 * shared between many threads. It consists from three classes WeakRef<T>, WeakRefTarget<T> and WeakRefPtr<T>.
 *
 * To start using weak references, you have to construct WeakRefTarget<T> which is initialized with a pointer
 * to an object which is subject of weak referencing. This object (WeakRefTarget) should be created along
 * with the target, because the target object must destroy this (WeakRefTarget) object (it should set NULL the
 * reference during destruction). Once the reference is set to NULL, all other references shared from that
 * target reference are also set to NULL. Note that, this is tje one way action. there is no way to set the pointer
 * back, or store there another value.
 *
 * Setting shared references to NULL is not easy task. Other threads can be somewhere in the middle of some
 * operation,which os repeatedly accessing the target object. Setting the reference to NULL would probably cause the program crash.
 * This is the reason, why the WeakRef doesn't provide direct dereference to the target object. Instead of this,
 * the reference must be locked during the time, when the thread accessing the target object. You
 * have to call the function lock() to obtain WeakRefPtr<T> which represent locked reference and provides
 * direct dereference to the target object. Once the reference is locked, it cannot be set to NULL. In this
 * case, setting the WeakRefTarget to null could potentially block, until the all locked references are released.
 *
 *
 */
template<typename T>
class WeakRef {

	static const atomicValue initedFlag = atomicValue(1) << (sizeof(atomicValue) * 8 - 1);
	static const atomicValue leftFlag = atomicValue(1) << (sizeof(atomicValue) * 8 - 2);

	class Pointer: public RefCntObj, public DynObject {
	public:
		///pointer to the target object
		T *ptr;
		///count of locks
		/** When MSB is set, the pointer is valid. When MSB is zero, the weak reference
		 * is reset to NULL (regadless on what ptr contains
		 *
		 * Rest of bits contain count of locks. Reference is unlocked when it reaches zero
		 */
		atomic lockCount;
		///during "reseting" phase, this is set to object which must be notified about unlocking the reference
		ISleepingObject *clearOutNotify;

		Pointer(T *ptr)
			:ptr(ptr)
			,lockCount(initedFlag)
			,clearOutNotify(0) {}
	};


public:

	///Constructs empty empty
	/** @note you have to avoid locking and unlocking empty reference until the value is set.
	 * */
	WeakRef() {}
	///Construct WeakRef
	/** Only way to construct weak ref is use other WeakRef instance to share the
	 * reference which they are hold. You can however share from WeakRefTarget, which
	 * can be initialized by value.
	 *
	 * @param other other WeakRef
	 */
	WeakRef(const WeakRef &other):ref(other.ref) {}


	WeakRefPtr<T> lock() const;

protected:

	friend class WeakRefPtr<T>;

	///Locks the reference
	/**
	 * The only way to obtain the pointer is throught function lock. Once the pointer is
	 * no longer needed, the function unlock() should be called. While the reference is locked,
	 * the referenced object cannot be destroyed. However, holding locked reference for a
	 * long time can cause a deadlock, because until the reference is unlocked, any destruction
	 * thread is delayed.
	 *
	 * @return Pointer to the object. Note that function can return NULL, once the object is no longer available.
	 *
	 * @see WeakRefPtr
	 */
	T *lockInternal() const {
		//increase lock count
		atomicValue v = lockInc(ref->lockCount);
		//test v for initedFlag - if set, return valid pointer
		if (v & initedFlag) return ref->ptr;
		//if not set, return 0;
		return 0;
	}
	///Unlocks the reference
	/** Unlocks the reference and releases any thread waiting for it. Count of unlocks must
	 * be equal to count of locks, otherwise undefined behavior may happen. To prevent many
	 * errors, you should use WeakRefPtr.
	 */
	void unlockInternal() const {
		//decrease count
		atomicValue v = lockDec(ref->lockCount);
		//test, whether the counter reched the zero
		if (v == 0)
			//if does, send notification
			ref->clearOutNotify->wakeUp(0);
	}

	RefCntPtr<Pointer> ref;

	WeakRef(T *init) {
		setPtr(init);
	}

	void setPtr(T *ptr) {
		ref = new(getWeakRefAllocator()) Pointer(ptr);
	}

	void setNull() {
		if (ref != null) {
			//function marks shared pointer as invalid (that why we are setting to null)
			//first, try straight attempt - assume, that pointer is not locked, so reset initFlag directly
			atomicValue v = lockCompareExchange(ref->lockCount,initedFlag,leftFlag);
			//if this was succesful, no more work can be done, everyone will see, that pointer is zero
			//if not, try plan B
			if (v != initedFlag && v != leftFlag) {
				//first obtain current thread sleeping object
				RefCntPtr<IThreadSleeper> sleeper = getCurThreadSleepingObj();
				//set sleeping object to the Pointer object. That because unlock() function can
				//send notification to the sleeper once the counter reaches zero
				ref->clearOutNotify =  sleeper;
				//now remove the initFlag bit simply by substracting the value from the counter
				//because we expect the only one thread that will destroy the target
				//nothing complex could be done
				v = lockExchangeSub(ref->lockCount, initedFlag);
				//substract it also localy
				v -= initedFlag;
				//hardest part
				//now the destroying thread must wait till the pointer is locked
				//because there are other threads that are currently working with the target
				while (v) {
					//we already ready to receive notification, halt thread now
					threadHalt();
					//thread woken, test, whether the pointer is unlocked
					//we do by this function to have interlocked access to the variable
					//it may cost performance, however eliminates any issues araising from dificulties while
					//working with shared variables
					v = lockExchangeAdd(ref->lockCount,0);
					//v should be zero now.
					//It is possible, that pointer has been locked again before the thread
					//gets chance to checking. No problem, because the pointer will be unlocked
					//soon which generates new wakeup notification
				}
				//now we woken and we are saw lockCount equal to zero
				//regadless on what happened next, the leftFlag can be set to
				//neutralize this object
				//because lockCount will never reach the zero, no more notifications
				//will be generated. We can safety destroy sleeper/thread/etc.
				lockExchangeAdd(ref->lockCount, leftFlag);
			}
		}
		ref = null;
	}

};

template<typename T>
class WeakRefTarget: public WeakRef<T> {
public:
	///Construct weak reference target
	/** target object has special purpose. Once the target object is destroyed, the all
	 * refereces all set to NULL. The target object is also source of reference and
	 * the only target object can be initialized by a pointer.
	 *
	 * @param v pointer to initialize WeakRefTarget
	 *
	 * @note WeakRefTarget has disabled assignment and copy constructor.
	 */
	WeakRefTarget(T *v):WeakRef<T>(v) {}

	///Set references to NULL
	/** this is not recomended, see the function setNull().
	 *
	 */
	~WeakRefTarget() {
		setNull();
	}
	///Makes refernece invalid settings all its instances to NULL.
	/**
	 * @note There is no way how to set value back. Once the reference is set to NULL,
	 * all references to the target are also set to NULL. The function eventually waits for
	 * locked references.
	 *
	 * You should call this function in the destructor of top most object, especially when
	 * the reference can access to the top most object's state. You can also let the destructor
	 * set the null automatically however this can happen too late and meanwhile other
	 * threads can access to a partially destroyed object.
	 *
	 * The function can be called even if the target has been set to null. Multiple calls
	 * are silently dropped. You should avoid call this function from multiple threads.
	 */

	void setNull() {
		WeakRef<T>::setNull();
	}

	///changes the target
	/** By assign a pointer to the WeakRefTarget invalidates current pointer
	 * similar to setNull. However, other WeakRefs will not receive new pointer. It
	 * requires to share the new pointer again.
	 *
	 * This useful, when you replace an old instance with new instance of the class T.
	 * It not good practice to replace instances while the other threads are using old
	 * instance. It is better to set old instance null and give new instance to newly
	 * created threads.
	 *
	 * @param ptr pointer to new instance. The value can be NULL, in such case, function
	 * works exactly same way as setNull()
	 *
	 * @return reference to this
	 */
	WeakRefTarget &operator=(T *ptr) {
		setNull();
		if (ptr) WeakRef<T>::setPtr(ptr);
		return *this;
	}



private:
	WeakRefTarget(const WeakRefTarget &other);
	WeakRefTarget &operator=(const WeakRefTarget &other);
};

template<typename T>
class WeakRefPtr: public Pointer<T> {
public:

	WeakRefPtr(const WeakRef<T> &ref)
		:Pointer<T>(ref.lockInternal()),me(ref) {}

	WeakRefPtr(const WeakRefPtr &other)
		:Pointer<T>(other.me.lockInternal()),me(other.me) {}

	~WeakRefPtr() {me.unlockInternal();}
protected:
	WeakRef<T> me;
};

template<typename T>
inline WeakRefPtr<T> LightSpeed::WeakRef<T>::lock() const {
	return WeakRefPtr<T>(*this);
}

}


#endif /* LIGHTSPEED_BASE_MEMORY_WEAKREF_H_ */
