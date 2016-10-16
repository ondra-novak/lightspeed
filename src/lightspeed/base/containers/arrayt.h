#pragma once
#include "../invokable.h"
#include "../qualifier.h"
#include "../types.h"
#include "../iter/iterator.h"

namespace LightSpeed {

template<typename T, typename A> class ArrayTRef;
template<typename A, typename B> class ArrSum;
template<typename A> class ArrMid;
template<typename A> class ArrRev;
template<typename A> class ArrRep;
template<typename A> class ArrExtend;
template<typename A, typename Fn> class ArrTrn;
template<typename A, typename NewT> class ArrCast;


template<typename T, typename Impl>
class ArrayT: public Invokable<Impl> {
public:

	typedef T ItemT;
	typedef typename OriginT<T>::T OrgItemT;
	typedef typename ConstObject<OrgItemT>::Add ConstItemT;
	typedef typename ConstObject<OrgItemT>::Remove NoConstItemT;
	typedef ConstItemT &ConstRef;
	typedef OrgItemT &Ref;

	typedef ArrMid<const Impl &> ConstMid;
	typedef ArrMid<Impl &> Mid;
	typedef ArrRev<const Impl &> ConstRev;
	typedef ArrRev<Impl &> Rev;
	typedef ArrRep<const Impl &> ConstRep;
	typedef ArrRep<Impl &> Rep;
	typedef ArrExtend<const Impl &> ConstExtend;
	typedef ArrExtend<Impl &> Extend;

	template<typename X>
	struct TT {
		typedef ArrSum<const Impl &, const X &> ConstSum;
		typedef ArrSum<Impl &, X &> Sum;
		typedef ArrTrn<const Impl &, X> ConstTrn;
		typedef ArrTrn<Impl &, X> Trn;
	};

	natural length() const {return this->_invoke().length();}

	ConstRef at(natural index) const {
		return this->_invoke().at(index);
	}

	Ref mutableAt(natural index)  {
		return this->_invoke().mutableAt(index);
	}

	Impl &set(natural index, ConstRef val) {
		return this->_invoke().set(index,val);
	}

	ConstRef operator[](natural index) const {
		return at(index);
	}

	Ref operator()(natural index) {
		return mutableAt(index);
	}

	template<typename Fn>
	bool forEach(Fn functor) const {
		return this->_invoke().forEach(functor, 0, length());
	}

	template<typename Fn>
	bool forEach(Fn functor, natural from, natural count) const {
		natural l = length();
		if (from > l) return true;
		if (from + count > l) count = l - count;
		if (count == 0) return true;
		return this->_invoke().forEach(functor, from, count);
	}

	class Iterator: public IteratorBase<ItemT, Iterator> {
	public:
		typedef ArrayT Impl2;

		Iterator(const Impl2 &impl, Direction::Type dir)
			:pos(dir==Direction::backward?impl.length()-1:0),impl(impl),dir(dir) {}
		bool hasItems() const {
			return (pos < impl.length());
		}

		natural getRemain() const {
			return dir == Direction::backward?pos+1:impl.length()-pos;
		}
		ConstRef getNext() {
			if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(ItemT));
			const ItemT &val = impl[pos];
			switch(dir) {
				case Direction::forward: pos++;break;
				case Direction::backward: pos--;break;
				default:break;

			}
			return val;
		}
		ConstRef peek() const {
			if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(ItemT));
			return impl[pos];
		}
		bool equalTo(const Iterator &other) const {
			return (&impl == &other.impl && pos == other.pos);
		}
		bool lessThan(const Iterator &other) const {
			return (&impl == &other.impl && pos < other.pos);
		}

		natural seek(natural offset, Direction::Type direction) {
			natural sz = impl.length();
			switch(direction) {
				case Direction::current: return seek(offset,dir);
				case Direction::reversed: return seek(offset,Direction::reverse(dir));
				case Direction::forward:
					if (offset == 0) return 0;
					if (pos + offset > sz) offset = sz - pos;
					pos+=offset;
					return offset;
				case Direction::backward:
					if (offset == 0) return 0;
					if (pos+1 > 0) offset = pos+1;
					pos-= offset;
					return offset;
				case Direction::absolute:
					if (offset > sz && offset != naturalNull) offset = sz;
					pos = offset;
					return offset;
				default: return 0;
			}
		}

		natural tell() const {
			return pos;
		}

		bool canSeek(Direction::Type direction) const {
			natural sz = impl.length();
			switch(direction) {
				case Direction::current:return canSeek(dir);
				case Direction::reversed:return canSeek(Direction::reverse(dir));
				case Direction::forward:return pos >= sz;
				case Direction::backward:return pos != naturalNull;
				case Direction::absolute:return true;
				default: return false;
			}
		}

		natural getRemain(const Direction::Type direction) const {
			natural sz = impl.length();
			switch(direction) {
				case Direction::current:return getRemain(dir);
				case Direction::reversed:return getRemain(Direction::reverse(dir));
				case Direction::forward:return sz - pos;
				case Direction::backward:return pos + 1;
				default: return 0;
			}
		}


	protected:
		natural pos;
		const Impl2 &impl;
		Direction::Type dir;

	};

	///Returns forward iterator for all items
	Iterator getFwIter() const {
		return Iterator(*this, Direction::forward);
	}
	///Returns backward iterator for all items
	Iterator getBkIter() const {
		return Iterator(*this, Direction::backward);
	}

	static CompareResult compareItems(ConstRef a, ConstRef b) {
		return Impl::compareItems(a,b);
	}

	template<typename X>
	CompareResult compare(const ArrayT<T,X> &other) const {
		return this->_invoke().compare(other);
	}

	template<int n>
	CompareResult compare(ItemT (&arr)[n]) const;

	template<typename X> bool operator ==(const ArrayT<T,X> &other) const {
		return compare(other) == cmpResultEqual;
	}
	template<typename X> bool operator !=(const ArrayT<T,X> &other) const {
		return compare(other) != cmpResultEqual;
	}
	template<typename X> bool operator >=(const ArrayT<T,X> &other) const {
		return compare(other) != cmpResultLess ;
	}
	template<typename X> bool operator <=(const ArrayT<T,X> &other) const {
		return compare(other) != cmpResultGreater;
	}
	template<typename X> bool operator >(const ArrayT<T,X> &other) const {
		return compare(other) == cmpResultGreater ;
	}
	template<typename X> bool operator <(const ArrayT<T,X> &other) const {
		return compare(other) == cmpResultLess;
	}
	template<int n> bool operator ==(ItemT (&arr)[n]) const {
		return compare(arr) == cmpResultEqual;
	}
	template<int n>	bool operator !=(ItemT (&arr)[n]) const {
		return compare(arr) != cmpResultEqual;
	}
	template<int n> bool operator >=(ItemT (&arr)[n]) const {
		return compare(arr) != cmpResultLess ;
	}
	template<int n> bool operator <=(ItemT (&arr)[n]) const {
		return compare(arr) != cmpResultGreater;
	}
	template<int n> bool operator >(ItemT (&arr)[n]) const {
		return compare(arr) == cmpResultGreater ;
	}
	template<int n>	bool operator <(ItemT (&arr)[n]) const {
		return compare(arr) == cmpResultLess;
	}
	template<class UTr> bool dependOn(const ArrayT<T,UTr> *instance) const {
		return this->_invoke().dependOn(instance);
	}
	template<typename Oth>
	typename TT<Oth>::Sum concat(ArrayT<T,Oth> &o) {
		return typename TT<Oth>::Sum(this->_invoke(),o._invoke());
	}
	template<typename Oth>
	typename TT<Oth>::ConstSum concat(const ArrayT<T,Oth> &o) const {
		return typename TT<Oth>::ConstSum(this->_invoke(),o._invoke());
	}

	template<typename Oth>
	typename TT<Oth>::Sum operator+(ArrayT<ConstItemT,Oth> &o) {
		return typename TT<Oth>::Sum(this->_invoke(),o._invoke());
	}
	template<typename Oth>
	typename TT<Oth>::ConstSum operator+(const ArrayT<ConstItemT,Oth> &o) const {
		return typename TT<Oth>::ConstSum(this->_invoke(),o._invoke());
	}
/*	template<typename Oth>
	typename TT<Oth>::Sum operator+(ArrayT<NoConstItemT,Oth> &o) {
		return typename TT<Oth>::Sum(this->_invoke(),o._invoke());
	}*/
	template<typename Oth>
	typename TT<Oth>::ConstSum operator+(const ArrayT<NoConstItemT,Oth> &o) const {
		return typename TT<Oth>::ConstSum(this->_invoke(),o._invoke());
	}
	Mid mid(natural pos, natural count)  {
		natural l = this->_invoke().length();
		if (pos >= l) {pos = 0;count = 0;}
		else if (pos + count > l) count = l - pos;
		return Mid(this->_invoke(),pos,count);
	}
	ConstMid mid(natural pos, natural count) const {
		natural l = this->_invoke().length();
		if (pos >= l) {pos = 0;count = 0;}
		else if (pos + count > l) count = l - pos;
		return ConstMid(this->_invoke(),pos,count);
	}
	Mid head(natural count) {
		if (count > this->_invoke().length()) count = this->_invoke().length();
		return Mid(this->_invoke(),0,count);
	}
	ConstMid head(natural count) const {
		if (count > this->_invoke().length()) count = this->_invoke().length();
		return ConstMid(this->_invoke(),0,count);
	}
	Mid offset(natural pos)  {
		if (pos > this->_invoke().length()) pos = this->_invoke().length();
		return Mid(this->_invoke(),pos,this->_invoke().length() - pos);
	}
	ConstMid offset(natural pos) const {
		if (pos > this->_invoke().length()) pos = this->_invoke().length();
		return ConstMid(this->_invoke(),pos,this->_invoke().length() - pos);
	}
	Mid tail(natural pos)  {
		if (pos > this->_invoke().length()) pos = this->_invoke().length();
		return Mid(this->_invoke(),this->_invoke().length()-pos,pos);
	}
	ConstMid tail(natural pos) const {
		if (pos > this->_invoke().length()) pos = this->_invoke().length();
		return ConstMid(this->_invoke(),this->_invoke().length()-pos,pos);
	}
	Mid roffset(natural pos)  {
		if (pos > this->_invoke().length()) pos = this->_invoke().length();
		return Mid(this->_invoke(),0,this->_invoke().length() - pos);
	}
	ConstMid roffser(natural pos) const {
		if (pos > this->_invoke().length()) pos = this->_invoke().length();
		return ConstMid(this->_invoke(),0,this->_invoke().length() - pos);
	}
	Mid crop(natural left, natural right) {
		natural l = this->_invoke().length();
		if (left+right >= l) return Mid(this->_invoke(),0,0);
		else return Mid(this->_invoke(),left,l-right-left);
	}
	ConstMid crop(natural left, natural right) const {
		natural l = this->_invoke().length();
		if (left+right >= l) return ConstMid(this->_invoke(),0,0);
		else return ConstMid(this->_invoke(),left,l-right-left);
	}
	template<typename Oth>
	Mid trim(const ArrayT<T,Oth> &trimItems, Direction::Type dir = Direction::undefined) {
		natural beg = 0;
		natural end = this->_invoke().length();
		if (dir != Direction::right) {
			while (beg < end && trimItems.find(this->at(beg)) != naturalNull) beg++;
		}
		if (dir != Direction::left) {
			while (beg < end && trimItems.find(this->at(end-1)) != naturalNull) end--;
		}
		return mid(beg, end-beg);
	}
	template<typename Oth>
	ConstMid trim(const ArrayT<T,Oth> &trimItems, Direction::Type dir = Direction::undefined) const {
		natural beg = 0;
		natural end = this->_invoke().length();
		if (dir != Direction::right) {
			while (beg < end && trimItems.find(this->at(beg)) != naturalNull) beg++;
		}
		if (dir != Direction::left) {
			while (beg < end && trimItems.find(this->at(end-1)) != naturalNull) end--;
		}
		return mid(beg, end-beg);
	}

	Mid trim(const T &trimItem, Direction::Type dir = Direction::undefined) {
		natural beg = 0;
		natural end = this->_invoke().length();
		if (dir != Direction::right) {
			while (beg < end && trimItem == this->at(beg)) beg++;
		}
		if (dir != Direction::left) {
			while (beg < end && trimItem == this->at(end-1)) end--;
		}
		return mid(beg, end-beg);
	}

	ConstMid trim(const T &trimItem, Direction::Type dir = Direction::undefined) const {
		natural beg = 0;
		natural end = this->_invoke().length();
		if (dir != Direction::right) {
			while (beg < end && trimItem == this->at(beg)) beg++;
		}
		if (dir != Direction::left) {
			while (beg < end && trimItem == this->at(end-1)) end--;
		}
		return mid(beg, end-beg);
	}

	Extend extend(ConstRef fillChar, natural begin, natural end) {
		return Extend(*this,fillChar,begin,end);
	}
	ConstExtend extend(ConstRef fillChar, natural begin, natural end) const {
		return ConstExtend(*this,fillChar,begin,end);
	}
	Rev reverse() {
		return Rev(*this);
	}

	ConstRev reverse() const {
		return ConstRev(*this);
	}

	template<typename Fn>
	typename TT<Fn>::Trn transform(const Fn fn) {
		return typename TT<Fn>::Trn(*this,fn);
	}

	template<typename Fn>
	typename TT<Fn>::ConstTrn transform(const Fn &fn) const {
		return typename TT<Fn>::ConstTrn(*this,fn);
	}



	Rep repeat(natural count) {
		return Rep(*this, count);
	}

	ConstRep repeat(natural count) const {
		return Rep(*this, count);
	}
	ArrayTRef<T,Impl &> ref() {
		return ArrayTRef<T,Impl &>(this->_invoke());
	}
	ArrayTRef<T,typename ConstObject<Impl>::Add &> ref() const {
		return ArrayTRef<T,typename ConstObject<Impl>::Add &>(this->_invoke());
	}

	ArrayTRef<const T,typename ConstObject<Impl>::Add &> constRef() const {
		return ArrayTRef<const T,typename ConstObject<Impl>::Add &>(this->_invoke());
	}
	template<typename X>
	ArrCast<typename ConstObject<Impl>::Add &, X> cast() const {
		return ArrCast<typename ConstObject<Impl>::Add &, X>(*this);
	}
	template<typename X>
	ArrCast<Impl&,X> cast() {
		return ArrCast<Impl &,X>(*this);
	}

	void safeDestroy() {
		class Destroyer {
		public:
			Destroyer(ArrayT &arr):arr(arr),index(0),len(arr.length()) {}
			void run() {
				while (index<len) arr(index++).~ItemT();
			}
			~Destroyer() {run();}

		protected:
			ArrayT &arr;
			natural index;
			natural len;
		};
		Destroyer d(*this);
		d.run();
	}
	bool empty() const {
		return this->_invoke().empty();
	}
private:
    class FindItemFunctor {
    public:
    	FindItemFunctor(const ItemT &item, natural &pos):item(item),pos(pos) {}
    	bool operator()(const ItemT &x) const {
    		if (compareItems(x,item) == cmpResultEqual) return true;
    		++pos;
    		return false;
    	}
    protected:
    	const ItemT &item;
    	natural &pos;
    };

    template<typename X>
    class FindOneOfFunctor {
    public:
    	FindOneOfFunctor(const ArrayT<T,X> &item, natural &pos):item(item),pos(pos) {}
    	bool operator()(const ItemT &x) const {
    		if (item.find(x) != naturalNull) return true;
    		++pos;
    		return false;
    	}
    protected:
    	const ArrayT<T,X> &item;
    	natural &pos;
    };

    template<typename Itr>
    class RenderFunctor {
    public:
    	RenderFunctor(Itr &itr, natural &cnt):itr(itr),cnt(cnt) {}
    	bool operator()(const ItemT &x) const {
    		if (!itr.hasItems()) return true;
    		++cnt;
    		itr.write(x);
    		return false;
    	}
    protected:
    	Itr &itr;
    	natural &cnt;
    };

    template<typename X>
    class RenderToArrFunctor {
    public:
    	RenderToArrFunctor(ArrayTRef<T,X> arr, natural &cnt):arr(arr),cnt(cnt),len(arr.length()) {}
    	bool operator()(const ItemT &x) const {
    		if (cnt + 1 >= len) return false;
    		arr(++cnt) = x;
    		return true;
    	}
    protected:
    	ArrayTRef<T,X> arr;
    	natural &cnt;
    	natural len;
    };

public:
    natural find(const ItemT &item) const {
    	if (empty()) return naturalNull;
    	natural pos = 0;
    	if (!forEach(FindItemFunctor(item,pos))) return naturalNull;
    	return pos;
    }
    natural find(const ItemT &item, natural pos) const {
    	natural r = offset(pos).find(item);
    	return r == naturalNull?r:r+pos;
    }

    template<typename Oth>
    natural find(const ArrayT<T,Oth> &other) const{
    	natural l = length();
    	natural ol = other.length();
    	if (ol == 0 || ol > l) return naturalNull;
    	natural pos = find(other[0]);
    	while ( pos != naturalNull) {
    		if (mid(pos,ol) == other) return pos;
    		
    		pos = find(other[0],pos+1);
    	}
    	return naturalNull;
    }

    template<typename Oth>
    natural find(const ArrayT<T,Oth> &other, natural pos) const{
    	natural r = offset(pos).find(other);
    	return r == naturalNull?r:r+pos;
    }


    template<typename Oth>
    natural findOneOf(const ArrayT<T,Oth> &items) const {
    	if (empty()) return naturalNull;
    	natural pos = 0;
    	if (!forEach(FindOneOfFunctor<Oth>(items,pos))) return naturalNull;
    	return pos;
    }

    template<typename Oth>
    natural findOneOf(const ArrayT<T,Oth> &other, natural pos) const{
    	natural r = offset(pos).findOneof(other);
    	return r == naturalNull?r:r+pos;
    }

    natural findLast(const ItemT &item) const {
    	if (empty()) return naturalNull;
    	natural pos = 0;
    	if (!offset(0).reverse().forEach(FindItemFunctor(item,pos))) return naturalNull;
    	return length() - pos - 1;
    }

    template<typename Oth>
    natural findLast(const ItemT &other, natural pos) const{
    	natural r = head(pos).findLast(other);
    	return r;
    }


    template<typename Oth>
    natural findLast(const ArrayT<T,Oth> &other) const{
    	natural p = offset(0).reverse().find(other.offset(0).reverse());
    	if (p == naturalNull) return p;
    	return length() - p - 1;
    }


    template<typename Oth>
    natural findLast(const ArrayT<T,Oth> &other, natural pos) const{
    	natural r = head(pos).findLast(other);
    	return r;
    }

    template<typename Oth>
    natural findLastArr(const ArrayT<T,Oth> &other) const{
    	natural p = offset(0).reverse().find(other.offset(0).reverse());
    	if (p == naturalNull) return p;
    	return length() - p - 1;
    }

    template<typename Oth>
    natural findLastOneOf(const ArrayT<T,Oth> &items) const {
    	if (empty()) return naturalNull;
    	natural pos = 0;
    	if (!offset(0).reverse().forEach(FindOneOfFunctor<Oth>(items,pos))) return naturalNull;
    	return length() - pos - 1;
    }

    template<typename Oth>
    natural findLastOneOf(const ArrayT<T,Oth> &other, natural pos) const{
    	natural r = head(pos).findLastOneOf(other);
    	return r;
    }

    template<typename X, typename Iter>
    natural render(IWriteIterator<X, Iter> &iter) const {
    	natural cnt = 0;
    	iter.reserve(length());
    	forEach(RenderFunctor<Iter>(iter._invoke(),cnt));
    	return cnt;
    }

    template<typename G>
    natural render(ArrayTRef<T,G> arr) const {
    	natural cnt = 0;
    	forEach(RenderToArrFunctor<G>(arr,cnt));
    	return cnt;
    }

	///Enumerates items separated by specified separator
	/**
	 * @see split
	 */
	class SplitIterator: public IteratorBase<ConstMid, SplitIterator > {
	public:

		SplitIterator(ConstRef separator, const ArrayT &subj)
			:separator(separator),subj(subj),tmpres(new(tmpresbuff) ConstMid(subj,0,0)),pos(0) {}
		~SplitIterator() {
			tmpres->~ConstMid();
		}

		bool hasItems() const {
			return pos <= subj.length();
		}

		const ConstMid &peek() const {
			if (!hasItems()) throwIteratorNoMoreItems(THISLOCATION,typeid(*this));
			natural p = subj.offset(pos).find(separator);
			tmpres->~ConstMid();
			if (p == naturalNull) {
				tmpres = new(tmpresbuff) ConstMid(subj,pos,subj.length() - pos);
			} else {
				tmpres = new(tmpresbuff) ConstMid(subj,pos,p);
			}
			return *tmpres;
		}

		const ConstMid &getNext()  {
			const ConstMid &res = peek();
			pos += res.length()+1;
			return res;
		}

		///Returns rest of the array
		const ConstMid &getRest() const {
			tmpres = new(tmpresbuff) ConstMid(subj, pos, subj.length() - pos);
			return *tmpres;
		}

		bool lessThan(const SplitIterator &other) const {
			return pos < other.pos;
		}

		bool equalTo(const SplitIterator &other) const {
			return pos == other.pos;
		}

	protected:
		typename ArrayT::ItemT separator;
		const ArrayT &subj;
		mutable byte tmpresbuff[sizeof(ConstMid)];
		mutable ConstMid *tmpres;
		natural pos;		

	};

	///Splits string into parts separated by separator
	/**
	 * @param separator how items are separated
	 * @return iterator which can be used to walk through all items
	 */
	SplitIterator split(ItemT separator) const {
		return SplitIterator(separator,*this);
	}
};

template<typename T>
class ArrAbstractFunction;

template<typename T, typename Impl>
class ArrayTBase: public ArrayT<T,Impl> {
public:
	typedef ArrayT<T,Impl> Super;

	typedef T ItemT;
	typedef typename Super::OrgItemT OrgItemT;
	typedef typename Super::ConstItemT ConstItemT;
	typedef typename Super::ConstRef ConstRef;
	typedef typename Super::Ref Ref;


	//using ArrayT<T,Impl>::forEach;

	natural length() const {ArrAbstractFunction<T> x; return 0;}
	ConstRef at(natural ) const {ArrAbstractFunction<T> x; throw;}
	Ref mutableAt(natural )  {ArrAbstractFunction<T> x; throw;}
	Impl &set(natural , ConstRef ) {ArrAbstractFunction<T> x; throw;}
	template<typename Fn>
	bool forEach(Fn functor, natural from, natural count) const {
		for (natural i = 0; i < count; i++) 
			if (functor(this->_invoke().at(i+from))) return true;
		return false;
	}

	template<typename Fn>
	bool forEach(Fn functor) const {
		return forEach(functor,0,this->_invoke().length());
	}

	static CompareResult compareItems(ConstRef a, ConstRef b) {
		if (a == b) return cmpResultEqual;
		else if (a < b) return cmpResultLess;
		else if (a > b) return cmpResultGreater;
		else return cmpResultNotEqual;
	}
	template<typename X>
	class CmpFunctor {
	public:
		CmpFunctor(const ArrayT<T,X> &other,CompareResult &cmpres)
			:cmpres(cmpres), other(other),index(0) {}
		bool operator()(const ItemT &x) const {
			cmpres = compareItems(x,other.at(index));
			++index;
			return cmpres != cmpResultEqual;
		}
		CompareResult &cmpres;

	protected:
		const ArrayT<T,X> &other;
		mutable natural index;
	};

	template<typename X>
	CompareResult compare(const ArrayT<T,X> &other) const {
		natural l1 = this->_invoke().length();
		natural l2 = other._invoke().length();
		if (l1 > l2) return swapCompareResult(other.compare(*this));
		if (l1 == 0) return l2 == 0?cmpResultEqual:cmpResultLess;
		CompareResult cr;
		forEach(CmpFunctor<X>(other,cr));
		if (cr == cmpResultEqual && l1 < l2) return cmpResultLess;
		else return cr;
	}

	template<class UTr> bool dependOn(const ArrayT<T,UTr> *instance) const {
		return instance == this;
	}

	bool empty() const {
		return this->_invoke().length() == 0;
	}

};

template<typename T, typename Impl>
class ArrayTBase<const T, Impl>: public ArrayT<T,Impl> {
public:
	typedef ArrayT<T,Impl> Super;

	typedef const T ItemT;
	typedef typename Super::OrgItemT OrgItemT;
	typedef typename Super::ConstItemT ConstItemT;
	typedef typename Super::ConstRef ConstRef;
	typedef typename Super::Ref Ref;




	template<typename Fn>
	bool forEach(Fn functor) const {
		return forEach(functor,0,this->_invoke().length());
	}

	template<typename Fn>
	bool forEach(Fn functor, natural from, natural count) const {
		for (natural i = 0; i < count; i++)
			if (functor(this->_invoke().at(i+from))) return true;
		return false;
	}
	static CompareResult compareItems(ConstRef a, ConstRef b) {
		if (a == b) return cmpResultEqual;
		else if (a < b) return cmpResultLess;
		else if (a > b) return cmpResultGreater;
		else return cmpResultNotEqual;
	}
	template<typename X>
	class CmpFunctor {
	public:
		CmpFunctor(const ArrayT<T,X> &other,CompareResult &cmpres)
			:cmpres(cmpres), other(other),index(0) {}
		bool operator()(const ItemT &x) const {
			cmpres = compareItems(x,other.at(index));
			++index;
			return cmpres != cmpResultEqual;
		}
		CompareResult &cmpres;

	protected:
		const ArrayT<T,X> &other;
		mutable natural index;
	};

	template<typename X>
	CompareResult compare(const ArrayT<T,X> &other) const {
		natural l1 = this->_invoke().length();
		natural l2 = other._invoke().length();
		if (l1 > l2) return swapCompareResult(other.compare(*this));
		if (l1 == 0) return l2 == 0?cmpResultEqual:cmpResultLess;
		CompareResult cr;
		forEach(CmpFunctor<X>(other,cr));
		if (cr == cmpResultEqual && l1 < l2) return cmpResultLess;
		else return cr;
	}

	template<class UTr> bool dependOn(const ArrayT<T,UTr> *instance) const {
		return instance == this;
	}

	bool empty() const {
		return this->_invoke().length() == 0;
	}
private:
	natural length() const {ArrAbstractFunction<T> x; return 0;}
	ConstRef at(natural ) const {ArrAbstractFunction<T> x; throw;}
	Ref mutableAt(natural )  {ArrAbstractFunction<T> x; throw;}
	Impl &set(natural , ConstRef ) {ArrAbstractFunction<T> x; throw;}

};

}

#include "arrayExpr.h"
