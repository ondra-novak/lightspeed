#ifndef LIGHTSPEED_STREAMS_COUNTERS_H_
#define LIGHTSPEED_STREAMS_COUNTERS_H_

#include "iterator.h"
#include "../qualifier.h"

namespace LightSpeed {
	
	///counting iterator 
	/**
	 * This is simple iterator works as proxy. It forwards requests to the iterator specified
	 * in the constructor, and as a side efect, it counts items transfered using it
	 * 
	 * @param NxIter specified next iterator. You can used both form NxIter and NxIter & depend on if
	 * 		you wish use copy of iterator or reference
	 * @param Counter specifies counter type. Default is natural. Sometimes it is better to use reference
	 * 		for example, when the iterator is copied as value. 
	 */
	template<class NxIter, class Counter = natural>
	class CountIterator: public IMutableIterator<typename DeRefType<NxIter>::T::ItemT,CountIterator<NxIter> > {
	public:
		typedef typename DeRefType<NxIter>::T::ItemT ItemT;
		
		CountIterator(NxIter nxIter,Counter counter):nxIter(nxIter),counter(counter) {}
		bool hasItems() const {return nxIter.hasItems();}
		natural getRemain() const {return nxIter.getRemain();}
		bool equalTo(const NxIter &other) const {return nxIter.equalTo(other);}
		bool lessThan(const NxIter &other) const {return nxIter.lessThan(other);}
		const ItemT &getNext() {
				const ItemT &res = nxIter.getNext();
				counter++;
				return res;
			}
		const ItemT &peek() {return nxIter.peek();}
		ItemT &getNextMutable() {
			ItemT &res = nxIter.getNextMutable();
			counter++;
			return res;
		}
        template<class Traits>
        natural blockRead(FlatArrayRef<ItemT,Traits> buffer, bool readAll) {
        	natural cnt = nxIter.blockRead(buffer,readAll);
        	counter+=cnt;
        	return cnt;
        }
        
        Counter getCounter() {return counter;}
        const Counter getCounter() const {return counter;}
        
		ItemT &peekMutable() const { return nxIter.peekMutable();}
	public:
		mutable NxIter nxIter;
		Counter counter;
		
	};

	///counting iterator
	/**
	 * This is simple write iterator works as proxy. It forwards requests to the iterator specified
	 * in the constructor, and as a side efect, it counts items transfered using it
	 * 
	 * @param NxIter specified next iterator. You can used both form NxIter and NxIter & depend on if
	 * 		you wish use copy of iterator or reference
	 * @param Counter specifies counter type. Default is natural. Sometimes it is better to use reference
	 * 		for example, when the iterator is copied as value. 
	 */
	template<class NxIter, class Counter = natural>
	class CountWriteIterator: public WriteIteratorBase<typename DeRefType<NxIter>::T::ItemT,CountWriteIterator<NxIter> > {
	public:
		typedef typename DeRefType<NxIter>::T::ItemT ItemT;
		
		CountWriteIterator(NxIter nxIter,Counter counter):nxIter(nxIter),counter(counter) {}
		CountWriteIterator(NxIter nxIter):nxIter(nxIter),counter(0) {}
		bool hasItems() const {return nxIter.hasItems();}
		natural getRemain() const {return nxIter.getRemain();}
		bool equalTo(const NxIter &other) const {return nxIter.equalTo(other);}
		bool lessThan(const NxIter &other) const {return nxIter.lessThan(other);}
		void write(const ItemT &x) {
			nxIter.write(x);
			counter++;
		}
        template<class Traits>
        natural blockWrite(const FlatArray<ItemT,Traits> &buffer, bool writeAll = true) {
        	natural cnt = nxIter.blockWrite(buffer,writeAll);
        	counter+=cnt;
        	return cnt;
        }

        Counter getCounter() {return counter;}
        const Counter getCounter() const {return counter;}
        
	public:
		mutable NxIter nxIter;
		Counter counter;
		
	};

}

#endif /*COUNTERS_H_*/
