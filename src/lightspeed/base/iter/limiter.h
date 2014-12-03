#pragma once

#include "iterator.h"

namespace LightSpeed {

	template<typename Iter, template<class,class> class BaseIter>
	class IteratorLimiterBase: 
		public BaseIter<typename OriginT<Iter>::T::ItemT, 
				IteratorLimiterBase<Iter,BaseIter> >
	{
	public:
		typedef typename OriginT<Iter>::T::ItemT T;

		IteratorLimiterBase(Iter iter, natural count, natural offset = 0):iter(iter),count(count) {
			while (offset && iter.hasItems()) {
				iter.skip();
				offset--;
			}
		}

		bool hasItems() const {return count > 0 && iter.hasItems();}
		natural getRemain() const {
			natural cur = iter.getRemain();
			if (cur < count) return cur; else return count;
		}
		const T &getNext() {
			if (count == 0) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			const T &r = iter.getNext();
			count--;
			return r;
		}

		const T &peek() const {
			if (count == 0) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			return iter.peek();
		}
		template<class Traits>
		natural blockRead(FlatArray<T,Traits> &buffer) {
			if (buffer.size() > count) {
				if (count == 0) return 0;
				return blockRead(buffer.mid(0,count));
			}
			natural res = iter.blockRead(buffer);
			count -= res;
			return res;
		}

		void skip(){
			if (count == 0) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			iter.skip();
			count--;
		}

		T &getNextMutable() {
			if (count == 0) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			 T &r = iter.getNextMutable();
			count--;
			return r;
		}

		T &peekMutable() const {
			if (count == 0) throwIteratorNoMoreItems(THISLOCATION,typeid(T));
			return iter.peekMutable();
		}

		void write(const T &x) {
			if (count == 0) throwWriteIteratorNoSpace(THISLOCATION,typeid(T));
			iter.write(x);
			count--;
		}
		bool canAccept(const T &x) const {
			return iter.canAccept(x);
		}

		template<class RdIterator>
		natural copy(RdIterator &input,natural limit = naturalNull) {
			if (limit > count) limit = count;
			natural res = iter.copy(input,limit);
			count-= res;
			return res;
		}

		template<class RdIterator>
		natural copyAccepted(RdIterator &input,natural limit = naturalNull) {
			if (limit > count) limit = count;
			natural res = iter.copyAccepted(input,limit);
			count-= res;
			return res;  
		}
		template<class Traits>
		natural blockWrite(const FlatArray<T,Traits> &buffer, bool writeAll) {
			if (buffer.size() > count) {
				if (count == 0) return 0;
				return blockRead(buffer.mid(0,count),writeAll);
			}
			natural res = iter.blockWrite(buffer,writeAll);
			count -= res;
			return res;
		}


	protected:
		Iter iter;
		natural count;
	};
	
	template<typename Iter>
	class IteratorLimiter: public  IteratorLimiterBase<Iter,IteratorBase> 
	{
	public:
		IteratorLimiter(Iter iter, natural count)
			:IteratorLimiterBase<Iter,::LightSpeed::IteratorBase> (iter,count) {}

	};

	template<typename Iter>
	class MutableIteratorLimiter: public  IteratorLimiterBase<Iter,MutableIteratorBase> 
	{
	public:
		MutableIteratorLimiter(Iter iter, natural count, natural offset = 0)
			:IteratorLimiterBase<Iter,::LightSpeed::MutableIteratorBase> (iter,count,offset) {}

	};

	template<typename Iter>
	class WriteIteratorLimiter: public  IteratorLimiterBase<Iter,WriteIteratorBase> 
	{
	public:
		WriteIteratorLimiter(Iter iter, natural count, natural offset = 0)
			:IteratorLimiterBase<Iter,::LightSpeed::WriteIteratorBase> (iter,count,offset) {}

	};
}


