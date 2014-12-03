#ifndef LIGHTSPEED_STREAMS_NULLSTREAM_H_
#define LIGHTSPEED_STREAMS_NULLSTREAM_H_
#include "iterator.h"

namespace LightSpeed {

	///Logical iterator which doesn't contain items
	/**
	 * This iterator is always empty. You can use this iterator everywhere is expected
	 * iterator and you don't want to supply any.
	 */ 
	template<class T>
	class NullIterator: public IMutableIterator<T,NullIterator<T> > {
	public:
		bool hasItems() const {return false;}
		natural getRemain() const {return 0;}
		bool equalTo(const NullIterator<T> &) const {return true;}
		bool lessThan(const NullIterator<T> &) const {return false;}
		const T &getNext()  {
			throwIteratorNoMoreItems(THISLOCATION, typeid(T));throw;
		}
		const T &peek() const {
			throwIteratorNoMoreItems(THISLOCATION, typeid(T));throw;
		}
        T &getNextMutable() {
			throwIteratorNoMoreItems(THISLOCATION, typeid(T));throw;
        }
        
        T &peekMutable() const {
			throwIteratorNoMoreItems(THISLOCATION, typeid(T));throw;
        }
	};
	
	///Logical write iterator which can act as black hole, or iterator with no space
	/**
	 * @param T type to write
	 * @param noBlackHole if true, iterator will throw exception on every write. Otherwise (false),
	 * 	iterator will act as black hole. Any written item will be ignored
	 */
	template<class T, bool blackHole>
	class NullWriteIterator: public WriteIteratorBase<T,NullWriteIterator<T,blackHole> > {
	public:
		bool hasItems() const {return blackHole;}
		natural getRemain() const {return blackHole?naturalNull:0;}
		bool equalTo(const NullIterator<T> &) const {return true;}
		bool lessThan(const NullIterator<T> &) const {return false;}
		
		void write(const T &/*x*/) {
			if (!blackHole) 
				throwWriteIteratorNoSpace(THISLOCATION,typeid(T));
			else 
				return;
		}
		template<class Traits>
		natural blockWrite(const FlatArray<typename ConstObject<T>::Remove,Traits> &buffer, bool ) {
			if (!blackHole) 
				throwWriteIteratorNoSpace(THISLOCATION,typeid(T));
			return buffer.length();
		}

		template<class Traits>
		natural blockWrite(const FlatArray<typename ConstObject<T>::Add,Traits> &buffer, bool ) {
			if (!blackHole) 
				throwWriteIteratorNoSpace(THISLOCATION,typeid(T));
			return buffer.length();
		}

	};

}	

#endif /*NULLSTREAM_H_*/
