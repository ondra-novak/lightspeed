#ifndef LIGHTSPEED_CONTAINERS_FLATARRAY_H_
#define LIGHTSPEED_CONTAINERS_FLATARRAY_H_
#include "arrayt.h"


namespace LightSpeed
{

	template<typename T, typename A>
	class FlatArrayRef;

	template<typename T, typename Impl2>
	class FlatArray_const:  public ArrayTBase<T, FlatArray<T,Impl2> >  {
	public:

		typedef ArrayT<T, FlatArray<T,Impl2> > Super;
		typedef typename Super::ItemT ItemT;
		typedef typename Super::OrgItemT OrgItemT;
		typedef typename Super::ConstItemT ConstItemT;
		typedef typename Super::NoConstItemT NoConstItemT;
		typedef typename Super::ConstRef ConstRef;
		typedef typename Super::Ref Ref;
		typedef typename ConstObject<Impl2>::Add ConstImpl2;


		///Returns pointer to derived class
		Impl2 &_invoke() {return *static_cast<Impl2 *>(this);}
		///Returns pointer to derived class 
		ConstImpl2 &_invoke() const {return *static_cast<ConstImpl2 *>(this);}

		
		
		natural length() const {
			return _invoke().length();
		}

		///retrieves pointer to first item in the array. Next item is at position+1
		/** Function returns const pointer because this function is const */
		ConstItemT *data() const {
			return _invoke().data();
		}


		///Receives pointer to first item of object that references other container
		/**
		 * @param ref pointer variable which receives return value. 
		 * Used to determine right prototype depend on expected return value. 
		 * Parameter can be const pointer or non-const pointer. Object forwards
		 * request to the referenced container. This allows function to be const
		 * even if requested pointer is not const.
		 */
		const ItemT *refData(const ItemT *ref) const {
			return _invoke().refData(ref);
		}


		FlatArrayRef<T,Impl2 &> ref() {
			return FlatArrayRef<T,Impl2 &>(this->_invoke());
		}
		FlatArrayRef<T,ConstImpl2 &> ref() const {
			return FlatArrayRef<T,ConstImpl2 &>(this->_invoke());
		}

		FlatArrayRef<ConstItemT,ConstImpl2 &> constRef() const {
			return FlatArrayRef<ConstItemT,ConstImpl2 &>(this->_invoke());
		}

		static CompareResult compareItems(ConstRef a, ConstRef b) {
			return Impl2::compareItems(a,b);
		}

		template<typename X>
		CompareResult compare(const ArrayT<ConstItemT,X> &other) const {
			return _invoke().compare(other);
		}
		template<typename X>
		CompareResult compare(const ArrayT<NoConstItemT,X> &other) const {
			return _invoke().compare(other);
		}

		ConstRef at(natural x) const {return data()[x];}

	};

	template<typename T, typename Impl2>
	class FlatArray_noconst:  public FlatArray_const<T,Impl2> {
	public:
		typedef  FlatArray_const<T,Impl2> Super;
		typedef typename Super::ItemT ItemT;
		typedef typename Super::ConstItemT ConstItemT;
		typedef typename Super::Ref Ref;
		typedef typename Super::ConstRef ConstRef;
		///retrieves pointer to first item in the array. Next item is at position+1
		/** Function returns non-const pointer because this function is not const*/
		ItemT *data() {
			return this->_invoke().data();
		}

		ConstItemT *data() const {
			return this->_invoke().data();
		}

		///Receives pointer to first item of object that references other container
		/**
		 * @param ref pointer variable which receives return value.
		 * Used to determine right prototype depend on expected return value.
		 * Parameter can be const pointer or non-const pointer. Object forwards
		 * request to the referenced container. This allows function to be const
		 * even if requested pointer is not const.
		 */

		ItemT *refData(ItemT *ref) const {
			return this->_invoke().refData(ref);
		}

		///Receives pointer to first item of object that references other container
		/**
		 * @param ref pointer variable which receives return value.
		 * Used to determine right prototype depend on expected return value.
		 * Parameter can be const pointer or non-const pointer. Object forwards
		 * request to the referenced container. This allows function to be const
		 * even if requested pointer is not const.
		 */
		const ItemT *refData(const ItemT *ref) const {
			return this->_invoke().refData(ref);
		}


		Ref mutableAt(natural x) {return data()[x];}
		Impl2 &set(natural x, ConstRef y) {data()[x] = y; return this->_invoke();}

		class WriteIterator: public WriteIteratorBase<T,WriteIterator> {
		public:
			WriteIterator(FlatArray_noconst &target, natural from):target(target),x(from) {}
			void write(const T &v) {
				if (x >= target.length()) throwWriteIteratorNoSpace(THISLOCATION,typeid(T));
				target.set(x,v);
				x++;
			}
			bool hasItems() const {
				return x < target.length();
			}

			natural getRemain() const {
				return target.length() - x;
			}

		protected:
			FlatArray_noconst &target;
			natural x;
		};

		WriteIterator getWriteIterator(natural at = 0) {
			return WriteIterator(*this,at);
		}

		template<typename X>
		CompareResult compare(const ArrayT<T,X> &other) const {
			return this->_invoke().compare(other);
		}

	};

	template<typename T, typename Impl2>
	class FlatArray: public FlatArray_noconst<T,Impl2> {};

	template<typename T, typename Impl2>
	class FlatArray<const T,Impl2>: public FlatArray_const<const T,Impl2> {};

	template<typename T, typename Impl2>
	class FlatArrayBase: public FlatArray<T,Impl2> {
	public:

		typedef FlatArray<T,Impl2> Super;
		typedef typename Super::ItemT ItemT;
		typedef typename Super::OrgItemT OrgItemT;
		typedef typename Super::ConstItemT ConstItemT;
		typedef typename Super::ConstRef ConstRef;
		typedef typename Super::Ref Ref;

		natural length() const {ArrAbstractFunction<T> x; return 0;}
		ConstItemT *data() const {ArrAbstractFunction<T> x; return 0;}
		ItemT *data() {ArrAbstractFunction<T> x; return 0;}
		OrgItemT *refData(OrgItemT *) const {ArrAbstractFunction<T> x; return 0;}
		ConstItemT  *refData(ConstItemT *) const {
			return this->_invoke().data();
		}
		static CompareResult compareItems(ConstRef a, ConstRef b) {
			return ArrayTBase<T,FlatArray<T,Impl2> >::compareItems(a,b);
		}

		template<typename X>
		CompareResult compare(const ArrayT<T,X> &other) const {
			return ArrayTBase<T,FlatArray<T,Impl2> >::compare(other);
		}
	};

	template<typename T, typename Impl2>
	class FlatArrayBase<const T, Impl2>: public FlatArray<const T,Impl2> {
	public:

		typedef FlatArray<const T,Impl2> Super;
		typedef typename Super::ItemT ItemT;
		typedef typename Super::OrgItemT OrgItemT;
		typedef typename Super::ConstItemT ConstItemT;
		typedef typename Super::ConstRef ConstRef;
		typedef typename Super::Ref Ref;

		natural length() const {ArrAbstractFunction<T> x; return 0;}
		ConstItemT *data() const {ArrAbstractFunction<T> x; return 0;}
		ItemT *data() {ArrAbstractFunction<T> x; return 0;}
		ConstItemT  *refData(ConstItemT *) const {
			return this->_invoke().data();
		}
		static CompareResult compareItems(ConstRef a, ConstRef b) {
			return ArrayTBase<const T,FlatArray<const T,Impl2> >::compareItems(a,b);
		}

		template<typename X>
		CompareResult compare(const ArrayT<T,X> &other) const {
			return ArrayTBase<const T,FlatArray<const T,Impl2> >::compare(other);
		}
		template<typename X>
		CompareResult compare(const ArrayT<const T,X> &other) const {
			return ArrayTBase<const T,FlatArray<const T,Impl2> >::compare(other);
		}
	};


	template<typename T, typename A>
	class FlatArrayRef: public FlatArrayBase<T,FlatArrayRef<T,A> > {
	public:
		typedef FlatArrayBase<T,FlatArrayRef<T,A> > Super;
		typedef typename ConstObject<typename Super::ItemT>::Remove MT;
		typedef typename ConstObject<typename Super::ItemT>::Add CT;

		FlatArrayRef(typename FastParam<A>::T x):x(x) {}

		natural length() const {return x.length();}
		typename Super::ConstItemT *data() const {
			const typename OriginT<A>::T &xx = x;
			return xx.data();
		}
		typename Super::ItemT *data() {return x.data();}

		MT *refData(MT *ref) const {
			return x.refData(ref);
		}
		CT *refData(CT *ref) const {
			return x.refData(ref);
		}

	protected:

		A x;

	};


	template<typename A>
	class FlatArrMid: public FlatArrayBase<typename OriginT<A>::T::ItemT,
										FlatArrMid<A> > {
	public:
		typedef typename FastParam<A>::T ArrRef;
		typedef FlatArrayBase<typename OriginT<A>::T::ItemT,
											FlatArrMid<A> > Super;
		typedef typename Super::ConstItemT ConstItemT;
		typedef typename Super::ItemT ItemT;
		typedef typename Super::NoConstItemT NoConstItemT;

		FlatArrMid(ArrRef a, natural start, natural len)
			:a(a),start(start),len(len) {}
		FlatArrMid(ArrRef a):a(a),start(0),len(a.length()) {}

		natural length() const {return len;}

		ConstItemT *data() const {
			const typename OriginT<A>::T &x = a;
			return x._invoke().data() + start;
		}

		ItemT *data() {
			return a._invoke().data() + start;
		}

		ItemT *refData(NoConstItemT *ref) const {
			return a._invoke().refData(ref) + start;
		}
		const ItemT *refData(const ItemT *ref) const {
			return a._invoke().refData(ref) + start;
		}

		A a;
		natural start,len;
	};



	template<typename T, typename Impl2>
	class ArrMid<FlatArray<T,Impl2> >: public FlatArrMid<FlatArray<T,Impl2> > {
	public:
		typedef typename FastParam<FlatArray<T,Impl2> >::T ArrRef;
		ArrMid(ArrRef a, natural start, natural len)
			:FlatArrMid<FlatArray<T,Impl2> >(a,start,len) {}

	};

	template<typename T, typename Impl2>
	class ArrMid<FlatArray<T,Impl2> &>: public FlatArrMid<FlatArray<T,Impl2> &> {
	public:
	typedef typename FastParam<FlatArray<T,Impl2> &>::T ArrRef;
		ArrMid(ArrRef a, natural start, natural len)
			:FlatArrMid<FlatArray<T,Impl2> &>(a,start,len) {}

	};

	template<typename T, typename Impl2>
	class ArrMid<const FlatArray<T,Impl2> &>: public FlatArrMid<const FlatArray<T,Impl2> &> {
	public:
	typedef typename FastParam<const FlatArray<T,Impl2> &>::T ArrRef;
		ArrMid(ArrRef a, natural start, natural len)
			:FlatArrMid<const FlatArray<T,Impl2> &>(a,start,len) {}

	};
} // namespace LightSpeed

#endif /*FLATARRAY_H_*/
