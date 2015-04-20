#pragma once

namespace LightSpeed {


	template<typename T, typename A> 
	class ArrayTRef: public ArrayTBase<T, ArrayTRef<T,A> > {
	public:

		typedef typename FastParam<A>::T ArrRef;
		typedef ArrayTBase<T,ArrayTRef<T,A> > Super;
		typedef typename Super::ConstRef ConstRef;
		typedef  ArrayTRef Impl;
		typedef typename Super::Ref Ref;

		ArrayTRef(ArrRef a):a(a) {}

		natural length() const {return a._invoke().length();}
		ConstRef at(natural index) const {
			return a._invoke().at(index);		
		}

		Ref mutableAt(natural index)  {
			return a._invoke().mutableAt(index);
		}

		Impl &set(natural index, ConstRef val) {
			return a._invoke().set(index,val);
		}

		template<typename Fn>
		bool forEach(Fn functor) const {
			return a._invoke().forEach(functor, 0, length());
		}

		template<typename Fn>
		bool forEach(Fn functor, natural from, natural count) const {
			return a._invoke().forEach(functor, from, count);
		}
		static CompareResult compareItems(ConstRef a, ConstRef b) {
			return a._invoke().compareItems(a,b);
		}
		template<typename X>
		CompareResult compare(const ArrayT<T,X> &other) const {
			return a._invoke().compareItems(a,other);
		}
		template<class UTr> bool dependOn(const ArrayT<T,UTr> *instance) const {
			return a._invoke().dependOn(instance);
		}
		bool empty() const {
			return a._invoke().empty();
		}

		ArrRef a;

	};

	template<typename A, typename B> 
	class ArrSum: public ArrayTBase<typename OriginT<A>::T::ItemT, ArrSum<A,B> > {
	public:

		typedef typename FastParam<A>::T ArrRefA;
		typedef typename FastParam<B>::T ArrRefB;
		typedef ArrayTBase<typename OriginT<A>::T::ItemT, ArrSum<A,B> > Super;
		typedef typename Super::ConstRef ConstRef;
		typedef ArrSum Impl;
		typedef typename Super::Ref Ref;
		typedef typename Super::ItemT ItemT;

		ArrSum(ArrRefA a, ArrRefB b):a(a),b(b) {}

		natural length() const {return a._invoke().length()+b._invoke().length();}
		ConstRef at(natural index) const {
			natural l = a._invoke().length();
			if (index >= l) return b._invoke().at(index-l);
			return a._invoke().at(index);		
		}

		Ref mutableAt(natural index)  {
			natural l = a._invoke().length();
			if (index >= l) return b._invoke().mutableAt(index-l);
			return a._invoke().mutableAt(index);
		}

		Impl &set(natural index, ConstRef val) {
			natural l = a._invoke().length();
			if (index >= l) {b._invoke().set(index-l,val);}
			else a._invoke().set(index,val);
			return *this;
		}
		
		template<typename Fn>
		bool forEach(Fn functor) const {
			return foreach(functor,0,length());
		}

		template<typename Fn>
		bool forEach(Fn functor, natural from, natural count) const {
			natural l = a._invoke().length();
			if (from >= l) return b._invoke().forEach(functor,from-l,count);
			else if (from + count <= l) return a._invoke().forEach(functor,from,count);
			else return (
				a._invoke().forEach(functor,from,l-from) 
								|| b._invoke().forEach(functor,0,count - l));

		}
		
		template<class UTr> bool dependOn(const ArrayT<ItemT,UTr> *instance) const {
			return a._invoke().dependOn(instance) || 
					b._invoke().dependOn(instance);
		}
		bool empty() const {
			return a._invoke().empty() && b._invoke().empty();
		}

		A a;
		B b;
	};
	template<typename A>
	class ArrMid: public ArrayTBase<typename OriginT<A>::T::ItemT, ArrMid<A> > {
	public:

		typedef typename FastParam<A>::T ArrRef;
		typedef ArrayTBase<typename OriginT<A>::T::ItemT, ArrMid<A> > Super;
		typedef typename Super::ConstRef ConstRef;
		typedef ArrMid Impl;
		typedef typename Super::Ref Ref;

		ArrMid(ArrRef a, natural start, natural len)
			:a(a),start(start),len(len) {}

		natural length() const {return len;}
		ConstRef at(natural index) const {
			return a._invoke().at(index+start);		
		}

		Ref mutableAt(natural index)  {
			return a._invoke().mutableAt(index+start);
		}

		Impl &set(natural index, ConstRef val) {
			return a._invoke().set(index+start,val);
		}

		template<typename Fn>
		bool forEach(Fn functor) const {
			return forEach(functor,0,length());
		}

		template<typename Fn>
		bool forEach(Fn functor, natural from, natural count) const {
			return a._invoke().forEach(functor, from+start, count);
		}
	
		bool empty() const {
			return len > 0;
		}

		ArrRef a;
		const natural start;
		const natural len;
	};

	template<typename A>
	class ArrMid<ArrMid<A> >: public ArrMid<A> {
	public:
		typedef ArrMid<A> Super;
		ArrMid(const Super &a, natural start, natural len):Super(a.a,start+a.start,len) {}
	};

	template<typename A>
	class ArrMid<ArrMid<A> &>: public ArrMid<A &> {
	public:
		typedef ArrMid<A> Super;
		ArrMid(Super &a, natural start, natural len):Super(a.a,start+a.start,len) {}
	};

	template<typename A>
	class ArrMid<const ArrMid<A> &>: public ArrMid<const A &> {
	public:
		typedef ArrMid<A> Super;
		ArrMid(const Super &a, natural start, natural len):Super(a.a,start+a.start,len) {}
	};

	template<typename A> 
	class ArrRev: public ArrayTBase<typename OriginT<A>::T::ItemT, ArrRev<A> > {
	public:
		typedef typename FastParam<A>::T ArrRef;
		typedef  ArrayTBase<typename OriginT<A>::T::ItemT, ArrRev<A> > Super;
		typedef typename Super::ConstRef ConstRef;
		typedef ArrRev Impl;
		typedef typename Super::Ref Ref;

		ArrRev(ArrRef a):a(a) {}

		natural length() const {return a._invoke().length();}
		ConstRef at(natural index) const {
			return a._invoke().at(reverseIndex(index));		
		}

		natural reverseIndex( natural index ) const
		{
			return a._invoke().length() - index - 1;
		}
		Ref mutableAt(natural index)  {
			return a._invoke().mutableAt(reverseIndex(index));
		}

		Impl &set(natural index, ConstRef val) {
			return a._invoke().set(reverseIndex(index),val);
		}

		template<typename Fn>
		bool forEach(Fn functor) const {
			return forEach(functor,0,length());
		}


		template<typename Fn>
		bool forEach(Fn functor, natural from, natural count) const {
			natural l = a._invoke().length();
			for (natural i = 0; i < count; i++) 
				if (functor(a._invoke().at(l - (i+from) - 1))) return true;
			return false;

		}

		A a;


	};
	template<typename A> 
	class ArrRep: public ArrayTBase<typename OriginT<A>::T::ItemT, ArrRep<A> > {
	public:
		typedef typename FastParam<A>::T ArrRef;
		typedef  ArrayTBase<typename OriginT<A>::T::ItemT, ArrRep<A> > Super;
		typedef typename Super::ConstRef ConstRef;
		typedef ArrRep Impl;
		typedef typename Super::Ref Ref;

		ArrRep(ArrRef a, natural count):a(a),count(count) {}

		natural length() const {return a._invoke().length() * count;}
		ConstRef at(natural index) const {
			return a._invoke().at(index % a._invoke().length());
		}

		Ref mutableAt(natural index)  {
			return a._invoke().mutableAt(index % a._invoke().length());
		}

		Impl &set(natural index, ConstRef val) {
			return a._invoke().set(index % a._invoke().length(),val);
		}

		using Super::forEach;

		template<typename Fn>
		bool forEach(Fn functor, natural from, natural count) const {
			natural l = a.length(), k = from % l;
			while (k + count > l) {
				if (a.forEach(functor,k,l-k)) return true;
				count -= l-k;
				k = 0;
			}
			return a.forEach(functor,k,count);
		}

		A a;
		const natural count;
	};


	enum ArrTrnSet { arrTrnSet };
	enum ArrTrnMutable { arrTrnMutable };

	template<typename A, typename Fn> 
	class ArrTrn: public ArrayTBase<typename OriginT<A>::T::ItemT, ArrTrn<A,Fn> > {
	public:
		typedef typename FastParam<A>::T ArrRef;
		typedef ArrayTBase<typename OriginT<A>::T::ItemT,ArrTrn<A,Fn> > Super;
		typedef typename Super::ConstRef ConstRef;
		typedef typename Super::NoConstItemT NoConstItemT;
		typedef ArrTrn Impl;
		typedef typename Super::Ref Ref;

		ArrTrn(ArrRef a, Fn fn):a(a),fn(fn),p(0) {}
		~ArrTrn() {
			if (p) p->~NoConstItemT();
		}

		natural length() const {return a._invoke().length();}
		ConstRef at(natural index) const {
			if (p) p->~NoConstItemT();
			p = new(buff)NoConstItemT(fn(a._invoke().at(index)));
			return *p;
		}

		template<typename FFn>
		bool forEach(FFn functor) const {
			return forEach(functor,0,length());
		}


		template<typename FFn>
		bool forEach(FFn functor, natural from, natural count) const {
			for (natural i = 0; i < count; i++)
				if (fn(functor(a._invoke().at(i+from)))) return true;
			return false;

		}

		A a;
		Fn fn;
	protected:
		mutable char buff[sizeof(NoConstItemT)];
		NoConstItemT *p;

	};

	template<typename A, typename NewT>
	class ArrCast: public ArrayTBase<NewT, ArrCast<A,NewT> > {
	public:
		typedef typename FastParam<A>::T ArrRef;
		typedef ArrayTBase<NewT, ArrCast<A,NewT> > Super;
		typedef typename Super::ConstRef ConstRef;
		typedef typename Super::Ref Ref;
		typedef ArrCast Impl;

		ArrCast(ArrRef a):a(a),p(0) {}

		natural length() const {return a._invoke().length();}
		ConstRef at(natural index) const {
			if (p) p->~NewT();
			return *new(buff) NewT(a.at(index));
		}

		Ref mutableAt(natural index)  {
			if (p) p->~NewT();
			return *new(buff) NewT(a.at(index));
		}

		Impl &set(natural index, ConstRef val) {
			a._invoke().set(index,val);
			return *this;
		}

		~ArrCast() {
			if (p) p->~NewT();
		}

		A a;
	protected:
		mutable char buff[sizeof(NewT)];
		NewT *p;

	};
}
