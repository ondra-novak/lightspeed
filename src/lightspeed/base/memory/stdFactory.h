/*
 * stdfactory.h
 *
 *  Created on: 6.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEE_MEMORY_STDFACTORY_H_
#define LIGHTSPEE_MEMORY_STDFACTORY_H_

#include "stdAlloc.h"
#include "factory.h"

namespace LightSpeed {

class StdFactory: public StdAlloc {
public:

	template<typename T>
	class Factory {
	public:
		T *createInstance() const {return new T();}
		T *createInstance(const T &src) const {return new T(src);}

		template<typename A1>
		T *createInstance(const A1 &p1) const {
			return new T(p1);
		}
		template<typename A1, typename A2>
		T *createInstance(const A1 &p1, const A2 &p2) const{
			return new T(p1,p2);
		}
		template<typename A1, typename A2, typename A3>
		T *createInstance(const A1 &p1, const A2 &p2, const A3 &p3) const{
			return new T(p1,p2,p3);
		}
		template<typename A1, typename A2, typename A3, typename A4>
		T *createInstance(const A1 &p1, const A2 &p2, const A3 &p3, const A4 &p4)const {
			return new T(p1,p2,p3,p4);
		}
		void destroyInstance(T *instance) const {
			delete instance;
		}


		Factory(const StdFactory &/*owner*/) {}
		template<typename X>
		Factory(const Factory<X> &/*owner*/) {}
		Factory(const Factory &/*owner*/) {}
		Factory() {}
	};



};

template<typename F>
class NoShareFactory: public F {
public:
	NoShareFactory(const NoShareFactory &/*owner*/):F() {}
	template<typename X>
	NoShareFactory(const NoShareFactory<X> &/*owner*/) {}
	NoShareFactory() {}

	template<typename T>
	class Factory: public F::template Factory<T> {
	public:
		typedef typename F::template Factory<T> Super;

		Factory(const Super &):Super(NoShareFactory()) {}
		template<typename X>
		Factory(const Factory<X> &):Super(NoShareFactory()) {}
		Factory(const Factory &):Super(NoShareFactory()) {}
		Factory(const NoShareFactory ):Super(NoShareFactory()) {}
		Factory() {}

	};

};

}

template<typename T>
void *operator new(size_t sz, LightSpeed::StdFactory::Factory<T> &) {
	return operator new(sz);
}

template<typename T>
void operator delete(void *ptr, LightSpeed::StdFactory::Factory<T> &) {
	operator delete(ptr);
}




#endif /* STDFACTORY_H_ */
