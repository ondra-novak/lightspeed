/*
 * atomic.h
 *
 *  Created on: 14.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_LINUX_ATOMIC_H_
#define LIGHTSPEED_MT_LINUX_ATOMIC_H_


#include "atomic_type.h"
#include "../../base/sync/trysynchronized.h"
#include "../../base/linux/seh.h"
#include "../../platform.h"
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
#include "llmutex.h"
#include <utility>
#endif
namespace LightSpeed {


#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
static LLMutex &atomicMutex() {
	static LLMutex mx;
	return mx;
}
#endif

	inline atomicValue lockCompareExchange(volatile  atomic &subj, atomicValue comparand, atomicValue value) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		if (subj == comparand) std::swap(subj,value); else value = subj;
		mx.unlock();
		return value;
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		__atomic_compare_exchange(&subj,&comparand,&value,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);
#else
		return __sync_val_compare_and_swap(&subj,comparand,value);
#endif
#endif
		return comparand;
	}

	inline atomicValue lockExchangeAdd(volatile  atomic &subj, atomicValue value) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		value = subj + value;
		std::swap(subj,value);
		mx.unlock();
		return value;
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		return __atomic_fetch_add(&subj,value,__ATOMIC_SEQ_CST);
#else
		return __sync_fetch_and_add(&subj,value);
#endif
#endif
		}

	inline atomicValue lockExchangeSub(volatile  atomic &subj, atomicValue value) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		value = subj - value;
		std::swap(subj,value);
		mx.unlock();
		return value;
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		return __atomic_fetch_sub(&subj,value,__ATOMIC_SEQ_CST);
#else
		return __sync_fetch_and_sub(&subj,value);
#endif
#endif
	}


	inline atomicValue lockInc(volatile atomic &subj) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		atomic ret = ++subj;
		mx.unlock();
		return ret;
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		return __atomic_add_fetch(&subj,1,__ATOMIC_SEQ_CST);
#else
		return __sync_add_and_fetch(&subj,1);
#endif
#endif
	}

	inline atomicValue lockDec(volatile atomic &subj) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		atomic ret = --subj;
		mx.unlock();
		return ret;
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		return __atomic_sub_fetch(&subj,1,__ATOMIC_SEQ_CST);
#else
		return __sync_sub_and_fetch(&subj,1);
#endif
#endif
	}

	inline atomicValue lockExchange(volatile atomic &subj, atomicValue value) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		std::swap(subj,value);
		mx.unlock();
		return value;
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		return __atomic_exchange_n(&subj,value,__ATOMIC_SEQ_CST);
#else
		atomic ret;
		do { ret = subj;} while (lockCompareExchange(subj,ret,value) != ret);
		return ret;
#endif
#endif
	}


	template<typename T>
	inline T *lockCompareExchangePtr( T *volatile *subj, T * comparand, T * value) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		T **x = (T**)subj;
		if (*subj == comparand) std::swap<T *>(*x,value); else value = *x;
		mx.unlock();
		return value;
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		(void)subj; //reported by some GCCs as unused argument;
		__atomic_compare_exchange(subj,&comparand,&value,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);
		return comparand;
#else
		return (T *)__sync_val_compare_and_swap(subj,comparand,value);
#endif
#endif
	}

	template<typename T>
	inline T *lockExchangePtr( T *volatile *subj, T * value) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		T **x = (T**)subj;
		std::swap<T *>(*x,value);
		mx.unlock();
		return value;
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		return __atomic_exchange_n(subj,value,__ATOMIC_SEQ_CST);
#else
		T *ret;
		do { ret = *subj;} while (lockCompareExchangePtr(subj,ret,value) != ret);
		return ret;
#endif
#endif
	}

	inline atomicValue readAcquire(const volatile atomic * var) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		atomic v = *var;
		mx.unlock();
		return v;
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		return __atomic_load_n(var,__ATOMIC_ACQUIRE);
#else
		__sync_synchronize();
		return *var;
#endif
#endif
	}

	template<typename T>
	inline T *readAcquirePtr(T * const volatile * var) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		T *v = *var;
		mx.unlock();
		return v;
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		return __atomic_load_n(var,__ATOMIC_ACQUIRE);
#else
		__sync_synchronize();
		return *var;
#endif
#endif
	}

	inline void writeRelease(volatile atomic *var, atomicValue val) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		*var = val;
		mx.unlock();
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		__atomic_store_n(var,val,__ATOMIC_RELEASE);
#else
		*var = val;
		__sync_synchronize();
#endif
#endif
	}

	template<typename T>
	inline void writeReleasePtr(T * volatile *var, T *val) {
#ifdef LIGHTSPEED_ATOMICS_USE_MUTEX
		LLMutex &mx = atomicMutex();
		mx.lock();
		*var = val;
		mx.unlock();
#else
#if HAVE_DECL___ATOMIC_COMPARE_EXCHANGE
		__atomic_store_n(var,val,__ATOMIC_RELEASE);
#else
		*var = val;
		__sync_synchronize();
#endif
#endif
	}





	template<typename T>
	T *safeGetPointerValue(T * & ptr) throw() {
		__seh_try {
			return ptr;
		} __seh_except (signum) {
			(void)signum;
			return 0;
		}
		throw;
	}

}

#endif /* LIGHTSPEED_MT_LINUX_ATOMIC_H_ */
