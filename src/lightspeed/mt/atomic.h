/*
 * atomic.h
 *
 *  Created on: 14.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_ATOMIC_H_
#define LIGHTSPEED_MT_ATOMIC_H_

#include "platform.h"

#ifdef LIGHTSPEED_PLATFORM_WINDOWS
#include "windows/atomic.h"
#endif

#ifdef LIGHTSPEED_PLATFORM_LINUX
#include "linux/atomic.h"
#endif
#include "../base/memory/pointer.h"


namespace LightSpeed {

	///Interlocked sets value of Pointer<T> object
	/**
	 * @param target target pointer
	 * @param test value compared with target pointer
	 * @param newval new value of pointer
	 * @retval true succcess
	 * @retval test failed
	 */
	template<typename T>
	bool lockSetPtrWhen(Pointer<T> &target, Pointer<T> test, Pointer<T> newval) {
		class PtrExtra: public Pointer<T> {
		public:
			bool lockSetPtrWhen(Pointer<T> test, Pointer<T> newval) {
				return lockCompareExchangePtr(&this->ptr,test.get(),newval.get()) == test.get();
			}
		};
		return static_cast<PtrExtra &>(target).lockSetPtrWhen(test,newval);
	}

	atomicValue lockCompareExchange( volatile atomic &subj, atomicValue comparand, atomicValue value);
	atomicValue lockExchangeAdd( volatile atomic &subj, atomicValue value);
	atomicValue lockExchangeSub( volatile atomic &subj, atomicValue value);
	atomicValue lockInc(volatile atomic &subj);
	atomicValue lockDec(volatile atomic &subj);

	///Atomically exchange value in subj
	/**
	 * @param subj shared variable which will be changed
	 * @param value new value stored into variable
	 * @return previous value
	 */
	atomicValue lockExchange(volatile atomic &subj, atomicValue value);
	template<typename T>
	T *lockCompareExchangePtr( T *volatile *subj, T * comparand, T * value);
	template<typename T>
	T *lockExchangePtr( T *volatile *subj,  T * value);

	atomicValue readAcquire(volatile const atomic * var);

	template<typename T>
	T *readAcquirePtr(T * const volatile * var);

	void writeRelease(volatile atomic *var, atomicValue val);

	template<typename T>
	void writeReleasePtr(T * volatile *var, T *val);


	template<typename T>
	T *safeGetPointerValue(T * & ptr) throw();
}



#endif /* LIGHTSPEED_MT_ATOMIC_H_ */
