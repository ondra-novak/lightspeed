/*
 * ptrallocator.h
 *
 *  Created on: 22.7.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_FACTORY_H_
#define LIGHTSPEED_FACTORY_H_

#pragma once

#include <typeinfo>
#include "../meta/isDynamic.h"
#include "../exceptions/throws.h"
#include "../invokable.h"

namespace LightSpeed {


	template<typename T, typename Impl>
	class FactoryBase: public Invokable<Impl> {
	public:


		T *createInstance() {
			void *p = this->_invoke().alloc(sizeof(T));
			try {
				return new(p) T;
			} catch (...) {
				this->_invoke().dealloc(p,sizeof(T));
				exceptionRethrow(THISLOCATION);
				throw;
			}
		}

		T *createIntastance(const T &src) {
			void *p = this->_invoke().alloc(sizeof(T));
			try {
				return new(p) T(src);
			} catch (...) {
				this->_invoke().dealloc(p,sizeof(T));
				exceptionRethrow(THISLOCATION);
				throw;
			}
		}

		template<typename A1>
		T *createInstance(const A1 &p1) {
			void *p = this->_invoke().alloc(sizeof(T));
			try {
				return new(p) T(p1);
			} catch (...) {
				this->_invoke().dealloc(p,sizeof(T));
				exceptionRethrow(THISLOCATION);
				throw;
			}
		}

		template<typename A1, typename A2>
		T *createInstance(const A1 &p1, const A2 &p2) {
			void *p = this->_invoke().alloc(sizeof(T));
			try {
				return new(p) T(p1,p2);
			} catch (...) {
				this->_invoke().dealloc(p,sizeof(T));
				exceptionRethrow(THISLOCATION);
				throw;
			}
		}

		template<typename A1, typename A2, typename A3>
		T *createInstance(const A1 &p1, const A2 &p2, const A3 &p3) {
			void *p = this->_invoke().alloc(sizeof(T));
			try {
				return new(p) T(p1,p2,p3);
			} catch (...) {
				this->_invoke().dealloc(p,sizeof(T));
				exceptionRethrow(THISLOCATION);
				throw;
			}
		}

		template<typename A1, typename A2, typename A3, typename A4>
		T *createInstance(const A1 &p1, const A2 &p2, const A3 &p3, const A4 &p4) {
			void *p = this->_invoke().alloc(sizeof(T));
			try {
				return new(p) T(p1,p2,p3,p4);
			} catch (...) {
				this->_invoke().dealloc(p,sizeof(T));
				exceptionRethrow(THISLOCATION);
				throw;
			}
		}

		void destroyInstance(T *instance) {
			if (instance == 0) return;
			natural sz = sizeof(T);
			try {
				instance->~T();
				this->_invoke().dealloc(instance,sz);
			} catch (...) {
				this->_invoke().dealloc(instance,sz);
				exceptionRethrow(THISLOCATION);
				throw;
			}
		}


		void *operatorNew(natural sz) {
			if (sz != sizeof(T))
				throwAllocatorLimitException(THISLOCATION, sz, sizeof(T), typeid(Impl));
			return this->_invoke.alloc(sizeof(T));
		}
		void operatorDelete(void *p, natural sz) {
			this->_invoke.dealloc(p,sz);
		}

	};


}

#endif /* PTRALLOCATOR_H_ */
