/*
 * threadObject.h
 *
 *  Created on: 24.1.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_SYNC_THREADOBJECT_H_
#define LIGHTSPEED_BASE_SYNC_THREADOBJECT_H_

#include "threadVar.h"
#include "../memory/stdFactory.h"

namespace LightSpeed {


	///Implements thread "signleton" - object is "virtually" allocated for every thread.
	/**
	 * Using this object, you are able to create object, which is available
	 * for every thread, but can have different state for each thread.
	 *
	 * Object is allocated on first access and remains until thread is exited
	 * @tparam T type stored by this object
	 * @tparam Alloc Allocator used to allocated instance
	 */
	template<typename T, typename Alloc = StdFactory>
	class ThreadObj {
	public:

		///Initializes object
		/**
		 * Class T must have default constructor accessible;
		 */
		ThreadObj():objfact(Alloc(),T()) {}
		///Initializes object using source object as template instance
		/**
		 * You can initialize ThreadObj by copy obj the instance of the class T
		 * @param obj an instance of class T
		 */
		ThreadObj(const T &obj):objfact(Alloc(),obj) {}
		///Initializes object using source object as template instance and specifies allocator instance
		/**
		 * @param obj an instance of class T
		 * @param a an instance of allocator
		 */
		ThreadObj(const T &obj, const Alloc &a):objfact(a,obj) {}

		///Retrieves reference to instance of class T for specified thread
		/**
		 * @param table reference to TLS table of the thread.
		 *
		 * @return reference to the instance. At first access, instance is allocated,
		 *   caller must be prepared, that exception can be thrown
		 */
		T &operator[](ITLSTable &table) {
			T *p = var[table];
			if (p == 0) {
				p = objfact.create();
				try {
					var.set(table,p,&objfact);
					return *p;
				} catch (...) {
					objfact.destroy(p);
					throw;
				}
			} else {
				return *p;
			}
		}

		///Directly access the object at current thread
		/**
		 * @return pointer to object, should never be NULL
		 * @note this function is slower in compare to braces[], because
		 * 		searching of TLS table may take a time. It is better to find
		 * 		out TLS table before accessing the object multiple-times
		 */
		T *operator->() {
			return &(*this)[ITLSTable::getInstance()];
		}
		T &operator *() {
			return (*this)[ITLSTable::getInstance()];
		}

	protected:
		typedef typename Alloc::template Factory<T> TFact;

		ThreadVar<T> var;

		class ObjectFactory: public IThreadVarDelete {
		public:
			ObjectFactory(const Alloc &alloc, const T &instance)
				:fact(alloc),templateInstance(instance) {}
			T *create() {
				return fact.createInstance(templateInstance);
			}
			virtual void operator()(void *ptr) {
				destroy(reinterpret_cast<T *>(ptr));
			}


			void destroy(T *itm) {fact.destroyInstance(itm);}

		protected:
			TFact fact;
			T templateInstance;
		};

		ObjectFactory objfact;
	};
}


#endif /* LIGHTSPEED_BASE_SYNC_THREADOBJECT_H_ */
