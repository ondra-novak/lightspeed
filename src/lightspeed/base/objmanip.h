
#pragma once
#include "types.h"
#include "meta/metaIf.h"
#include "meta/primitiveType.h"
#include <utility>
#include "containers/move.h"

namespace LightSpeed {

	void bincopy(void *target, const void *src, natural size);

	template<typename T> struct ObjManip;

	///Helps with manipulation with object especially in arrays
	/** some features are also better implemented in C++11. But
	 *  this template is still usage
	 *
	 *  Using global functions copy(), move(), construct() and destruct() can
	 *  invoke specialized operation for in-place construction or destruction
	 *  of the object.
	 */
	template<typename T> struct ObjManipInterface {
		///Copies and constructs array of objects
		/**
		 * @param src pointer to first object
		 * @param count count of object
		 * @param target target address where copies will be stored
		 * @return target casted to T *
		 *
		 * @note Target should point to uninitialized area. Function uses
		 * copy constructor of each object.e
		 *
		 * @exception any If constructor throw an exception, function rollbacks
		 *  copy operation destroying objects that has been copied. Note that
		 *  exception during destruction is treat as double exception error causing
		 *  abort of the program. Do not throw exception from destructors during
		 *  stack unwind.
		 *
		 *  @note function uses bincopy for POD objects and primitive types
		 */
		static T * copy(const T *src, natural count, void *target);
		///Copies one object
		/**
		 * @param src pointer to object to copy
		 * @param target pointer to uninitialized memory area
		 * @return target casted to T *
		 *
		 * @exception any any exceptions causes, that object on target area
		 * is treat as uninitalized
		 *
		 */
		static T * copyOne(const T *src, void *target);
		///Moves array of objects
		/**
		 * @param src source address
		 * @param count count of objects
		 * @param target target address with uninitialized aread
		 * @return target casted to T *
		 *
		 * @note after function returns, src points to uninitialized area. Function
		 * returns pointer to first object.
		 *
		 * @exception any if moving causes exception, function moves rollbacks operation.
		 *   Exception thrown during rollback is double exception error
		 *
		 */
		static T * move(T *src, natural count, void *target);

		///Move one object
		/**
		 * @param src address of the object
		 * @param target new address of the object.
		 * @return pointer to moved object
		 */
		static T * moveOne(const T *src, void *target);

		///Destroys array of object
		/**
		 * @param items pointer to an array
		 * @param count count of object
		 *
		 * After function returns, first argument points to an uninitialized area.
		 *
		 * @exception any exception thrown from the any destructor causes that
		 *  destruction will continue. Any exception thrown later is double
		 *  exception error. std::uncaught_exception is noticed when destruction
		 *  continues in exception state
		 */
		static void destruct(T *items, natural count);

		///Destroys an item
		static void destruct(T *item);

		///Constructs array of objects
		/**
		 * @param ptr pointer to uninitialized area
		 * @param count count of object to construct
		 * @param src value which will be used to initialize objects
		 * @return pointer to first object
		 *
		 * @exception any exception thrown from the any destructor causes rollback
		 * destroying all objects already constructed . exception during
		 * rollback is double exception error
		 */
		static T * construct(void *ptr, natural count, const T &src);

		///Constructs array of objects using default constructor
		/**
		 * @param ptr pointer to uninitialized area
		 * @param count count of object to construct
		 * @return pointer to first object
		 *
		 * @exception any exception thrown from the any destructor causes rollback
		 * destroying all objects already constructed . exception during
		 * rollback is double exception error
		 */
		static T * construct(void *ptr, natural count);

		static void swap(T *a1, T *a2, natural count);
	};

	template<typename T> struct ObjManipNonPOD: public ObjManipInterface<T> {
	public:

		//rollback with destruction
		class DestroyRollback{
		public:
			natural q; //< set this variable for every constructed item. Set to naturalNull to commit
			T *k;
			//requires pointer to begin of array
			DestroyRollback(T *k):q(0),k(k) {}
			//destroys when q is not set to naturalNull
			~DestroyRollback() {if (q+1) ObjManip<T>::destruct(k,q);}
		};

		class MoveRollback{
			T *k,*l;
			natural ll;
		public:
			natural q;  //< set this variable for every constructed item. Set to naturalNull to commit
			MoveRollback(T *k, T *l, natural ll):k(k),l(l),ll(ll),q(0) {}
			~MoveRollback() {
				if (q + 1) {
					//if target is less then source
					//moving has started from 0 to ll
					if (l < k) ObjManipNonPOD<T>::move(l,q,k);
					//otherwise, moving started at ll and continues to zero
					else ObjManipNonPOD<T>::move(l + q, ll - q, k + q);
				}
			}
		};

		class SwapRollback{
			T *k,*l;
		public:
			natural q;  //< set this variable for every constructed item. Set to naturalNull to commit
			SwapRollback(T *k, T *l):q(0),k(k),l(l) {}
			~SwapRollback() {
				if (q + 1) {
					ObjManip<T>::swap(k,l,q);
				}
			}
		};

		static T * copy(const T *src, natural count, void *target) {

			T *tt = reinterpret_cast<T *>(target);
			//destroy target when exception
			DestroyRollback r(tt);
			for (natural i = 0; i < count; i++) {
				r.q = i;//set rollback pos
				new (tt+i) T(src[i]);
			}
			r.q = naturalNull; //commit
			return tt;
		}

		static T *  copyOne(const T *src, void *target) {
			return new(target) T(*src);
		}

		static T *  move(T *src, natural count, void *target) {

			T *tpos = reinterpret_cast<T *>(target);
			//initialize rollback movement
			MoveRollback r(src,tpos,count);

			if (tpos < src) {
				for(natural i = 0; i < count; i++) {
					r.q = i;
					ObjManip<T>::moveOne(src+i,tpos+i);
				}
			} else {
				for(natural i = count; i > 0;) {
					r.q = i;
					i--;
					ObjManip<T>::moveOne(src+i,tpos+i);
				}
			}
			r.q = naturalNull; //commit
			return tpos;
		}


		static T *  moveOne(T *src, void *target) {
			MoveObject<T>::doMove(src,target);
/*			T *res = ObjManip<T>::copyOne(src,target);
			ObjManip<T>::destructOne(src);
			*/
			return reinterpret_cast<T *>(target);
		}

		static void destruct(T *items, natural count) {
			//initialize destroy rollback
			DestroyRollback r(items);
			//destroy items backward of construction
			//many algorithms works better when destruction is in reverse order
			//we can also use DestroyRollback
			for (natural i = count; i > 0; ) {
				i--;
				r.q = i; //mark item already destroyed - destruction cannot be rollbacked :-)
				ObjManip<T>::destructOne(items + i);
			}
			r.q = naturalNull; //commit
		}


		static void destructOne(T *item) {
			item->~T();
		}

		static T *  construct(void *ptr, natural count, const T &src) {

			T *tt = reinterpret_cast<T *>(ptr);
			DestroyRollback r(tt);
			for (natural i = 0; i < count; i++) {
				r.q = i;
				new (tt+i) T(src);
			}
			r.q = naturalNull;
			return tt;
		}

		static T *  construct(void *ptr, natural count) {
			T *tt = reinterpret_cast<T *>(ptr);
			DestroyRollback r(tt);
			for (natural i = 0; i < count; i++) {
				r.q = i;
				new (tt+i) T();
			}
			r.q = naturalNull;
			return tt;
		}

		static void swap(T *src, T *trg, natural count) {

			SwapRollback r(src,trg);
			for (natural i = 0; i < count; i++) {
				r.q = i;//set rollback pos
				std::swap(src[i],trg[i]);
			}
			r.q = naturalNull; //commit
		}
};

	template<typename T>
	void copy(const T *from, natural count, void *target) {
		ObjManip<T>::copy(from,count,target);
	}
	template<typename T>
	void copy(const T *from,  void *target) {
		ObjManip<T>::copyOne(from,target);
	}

	template<typename T>
	void move( T *from, natural count, void *target) {
		ObjManip<T>::move(from,count,target);
	}
	template<typename T>
	void move( T *from, void *target) {
		ObjManip<T>::moveOne(from,target);
	}


	template<typename T>
	void destruct(T *what, natural count) {
		ObjManip<T>::destruct(what,count);
	}

	template<typename T>
	void destruct(T *what) {
		ObjManip<T>::destructOne(what);
	}

	template<typename T>
	void construct( void *where, natural count, const T &src) {
		ObjManip<T>::construct(where,count,src);
	}

	template<typename T>
	void construct( void *where, natural count) {
		ObjManip<T>::construct(where,count);
	}

	template<typename T>
	void swap(T *from, T *to, natural count) {
		ObjManip<T>::swap(from,to,count);
	}

	///ObjManip for POD data
	template<typename T> struct ObjManipPOD: public ObjManipInterface<T> {
	public:
		///copies POD data using bincopy
		/**
		 * @param src source address
		 * @param count count of items
		 * @param target target address
		 *
		 * @note there will be no exception, no rollback is need
		 */
		static T * copy(const T *src, natural count, void *target) throw() {
			bincopy(target,src,count*sizeof(T));
			return reinterpret_cast<T *>(target);
		}

		///copies POD data using bincopy
		/**
		 * @param src source address
		 * @param target target address
		 */
		static T * copyOne(const T *src, void *target) {
			//it is better to use compiler generated copy constructor
			return new(target) T(*src);
		}

		static T * move(T *src, natural count, void *target) {
			return copy(src,count,target);
		}

		static T * moveOne(const T *src, void *target) {
			return copyOne(src,target);

		}

		///destruct is empty, because POD object has no destructor
		/** This help optimize destruction of primitive and POD data when optimization is not
		 *  perfectly done by compiler
		 */
		static void destruct(T *, natural ) {

		}

		///destructOne is empty, because POD object has no destructor
		/** This help optimize destruction of primitive and POD data when optimization is not
		 *  perfectly done by compiler
		 */
		static void destructOne(T *) {

		}

		static T * construct(void *ptr, natural count, const T &src) {
			for (natural i = 0; i < count ; i++)
				copyOne(&src,reinterpret_cast<T *>(ptr));
			return reinterpret_cast<T *>(ptr);
		}

		///Construct of array for POD is empty - because POD data has no constructor
		static T * construct(void *ptr, natural ) {
			return reinterpret_cast<T *>(ptr);
		}
		static void swap(T *src, T *trg, natural count) {

			for (natural i = 0; i < count; i++) {
				std::swap(src[i],trg[i]);
			}

		}


	};

	template<typename T> struct ObjManip: public  MIf<IsPrimitiveType<T>::value,ObjManipPOD<T>,ObjManipNonPOD<T> >::T {};
	

}


