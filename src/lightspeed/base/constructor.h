/*
 * constructor.h
 *
 *  Created on: 23.3.2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_CONSTRUCTOR_H_
#define LIGHTSPEED_BASE_CONSTRUCTOR_H_

namespace LightSpeed {

	///Construction delegation
	/** Performs construction at given address. Target object doesn't need to care about
	 * how many arguments is required for construction. Caller just creates this delegate
	 * as argument.
	 *
	 * @note Note that target object must support this type of construction. It have to
	 * declare function or constructor that accepts Constructor template class and
	 * calls construct() when it needs to construct that object. It is allowed to
	 * call function multiple-times depend on how many object need to by created
	 *
	 * There is also DefaultConstructor template that can be used to create object using
	 * default constructor. This is clean solution for templates that relates on
	 * template instantiation (where instantiaton can fail when target object doesn't
	 * support default constructor)
	 */
	template<typename T, typename Impl>
	class Constructor {
	public:

		typedef T ItemT;
		typedef Impl CImpl;

		T *construct(void *where) const {
			return static_cast<const Impl *>(this)->implConstruct(where);
		}
		const Constructor &base() const {return *this;}
	};

	///Construct using default constructor of T
	template<typename T>
	class DefaultConstructor: public Constructor<T,DefaultConstructor<T> > {
	public:
		DefaultConstructor() {}
		T *implConstruct(void *where) const {
			return new(where) T;
		}
	};

	///Construct using one argument
	template<typename T, typename K>
	class Constructor1: public Constructor<T,Constructor1<T,K> > {
	public:
		Constructor1(const K &val):val(val) {}
		T *implConstruct(void *where) const {
			return new(where) T(val);
		}
	protected:
		const K &val;
	};

	///Construct using two arguments
	template<typename T, typename K, typename L>
	class Constructor2: public Constructor<T,Constructor2<T,K,L> > {
	public:
		Constructor2(const K &val1,const L &val2):val1(val1),val2(val2) {}
		T *implConstruct(void *where) const {
			return new(where) T(val1,val2);
		}
	protected:
		const K &val1;
		const L &val2;
	};

	///Construct using three arguments
	template<typename T, typename K, typename L, typename M>
	class Constructor3: public Constructor<T,Constructor3<T,K,L,M> > {
	public:
		Constructor3(const K &val1,const L &val2,const M &val3):val1(val1),val2(val2),val3(val3) {}
		T *implConstruct(void *where) const {
			return new(where) T(val1,val2,val3);
		}
	protected:
		const K &val1;
		const L &val2;
		const M &val3;
	};

	///Construct using three arguments
	template<typename T, typename K, typename L, typename M, typename N>
	class Constructor4: public Constructor<T,Constructor4<T,K,L,M,N> > {
	public:
		Constructor4(const K &val1,const L &val2,const M &val3,const N &val4)
			:val1(val1),val2(val2),val3(val3),val4(val4) {}
		T *implConstruct(void *where) const {
			return new(where) T(val1,val2,val3,val4);
		}
	protected:
		const K &val1;
		const L &val2;
		const M &val3;
		const N &val4;
	};


	///Interface, which perform similar task as Constructor<T>, but using virtual binding
	/**
	 * Use this interface on places, where you cannot use Constructor<T> because
	 * for example you transferring the reference through a virtual function.
	 * This solution generates less optimal code, but it can be still better then
	 * copying object itself
	 *
	 * To feed argument of such type, use VtConstructor<> which construct
	 * object from original ConstructorT<>
	 */
	template<typename T>
	class IConstructor: public Constructor<T, IConstructor<T> > {
	public:
		virtual T *construct(void *where) const = 0;
		
		T *implConstruct(void *where) const {
			return this->construct(where);
		}
		
	};


	///Virtual function friendly type of the object Constructor<> based on IConstructor<>
	/**
	 * To create this object, create the Constructor<> object first and then use
	 * it as argument of the  constructor
	 */
	template<typename ConstructorT>
	class VtConstructor: public IConstructor<typename ConstructorT::ItemT>, public ConstructorT::CImpl {
	public:
		typedef typename ConstructorT::CImpl Super;
		typedef typename ConstructorT::ItemT T;
		VtConstructor(const ConstructorT &other):Super(static_cast<const Super &>(other)) {}
		virtual T *construct(void *where) const {return this->ConstructorT::construct(where);}
	};
}






#endif /* LIGHTSPEED_BASE_CONSTRUCTOR_H_ */
