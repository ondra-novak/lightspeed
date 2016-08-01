/*
 * atomic.h
 *
 *  Created on: 14.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_WINDOWS_ATOMIC_H_
#define LIGHTSPEED_MT_WINDOWS_ATOMIC_H_


#include "atomic_type.h"
#include "../../base/sync/trysynchronized.h"
namespace LightSpeed {




#pragma warning(disable:4197)


inline atomicValue lockCompareExchange( volatile atomic &subj, atomicValue  comparand, atomicValue value) {
#if _WIN64
	return InterlockedCompareExchange64(&subj,value,comparand);
#else
	return InterlockedCompareExchange(&subj,value,comparand);
#endif
}
inline atomicValue lockExchangeAdd( volatile atomic &subj, atomicValue value) {
#if _WIN64
	return InterlockedExchangeAdd64(&subj,value);
#else
	return InterlockedExchangeAdd(&subj,value);
#endif

}
inline atomicValue lockExchangeSub( volatile atomic &subj, atomicValue value) {
#if _WIN64
	return InterlockedExchangeAdd64(&subj,-value);
#else
	return InterlockedExchangeAdd(&subj,-value);
#endif
}
inline atomicValue lockInc(volatile atomic &subj) {
#if _WIN64
	return InterlockedIncrement64(&subj);
#else 
	return InterlockedIncrement(&subj);
#endif
}
inline atomicValue lockDec(volatile atomic &subj) {
#if _WIN64
	return InterlockedDecrement64(&subj);
#else 
	return InterlockedDecrement(&subj);
#endif

}

///Atomically exchange value in subj
/**
	* @param subj shared variable which will be changed
	* @param value new value stored into variable
	* @return previous value
	*/
inline atomicValue lockExchange(volatile atomic &subj, atomicValue value) {
#if _WIN64
	return InterlockedExchange64(&subj,value);
#else
	return InterlockedExchange(&subj,value);
#endif
}
template<typename T>
inline T *lockCompareExchangePtr( T *volatile *subj, T * comparand, T * value) {
	return (T *)InterlockedCompareExchangePointer((volatile PVOID *)subj,(PVOID)value,(PVOID)comparand);
}
template<typename T>
inline T *lockExchangePtr( T *volatile *subj,  T * value) {
	return (T *)InterlockedExchangePointer((volatile PVOID *)subj,(PVOID)value);
}

inline atomicValue readAcquire(volatile const atomic * var) {
	return *var;
}

template<typename T>
inline T *readAcquirePtr(T * const volatile * var) {
	return *var;
}
	

inline void writeRelease(volatile atomic *var, atomic val) {
	*var = val;
}

template<typename T>
inline void writeReleasePtr(T * volatile *var, T *val) {
	*var = val;
}


template<typename T>
inline T *safeGetPointerValue(T * & ptr) throw() {
	__try {
		return ptr;
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		return 0;
	}
}

}
#endif /* LIGHTSPEED_MT_WINDOWS_ATOMIC_H_ */
