#ifndef LIGHTSPEED_BUFFER_H_
#define LIGHTSPEED_BUFFER_H_

#include "autoArray.h"
#include "../qualifier.h" 
#include "../memory/staticAlloc.h"
#include "../exceptions/iterator.h"

namespace LightSpeed {

	///Implements reading buffer
	/** Class implements buffer used to store data temporary before they are readed by the iterator
	 * You should use buffering, when you working with iterator optimized for reading items by blocks. This
	 * is case of all I/O iterators, such a file streams, network streams, etc
	 * 
	 * ReadBuffer is iterator, which contains array, that is filled with data read using block reading of
	 * another iterator. When all items are read, buffer is automatically filled with next block of items
	 */
	template<class RdIterator, class Allocator = StdAlloc >
	class ReadBuffer: public IteratorBase<typename DeRefType<RdIterator>::T::ItemT, ReadBuffer<RdIterator,Allocator> > {
		
	public:
		typedef typename DeRefType<RdIterator>::T::ItemT ItemT;
		
		///Initializes buffer
		/** @param rdIter input iterator optimized for block reading
		 *  @param count size of buffer in the items
		 */
		ReadBuffer(RdIterator rdIter, natural count)
			:buffArr(count)
			,rdIter(rdIter),rdpos(0),wrpos(0),noItems(false) {}
		
		///Returns next item from the buffer
		/** @return next item from the buffer. Automatically loads items, if buffer is empty */		 
		const ItemT &getNext();		
		
		///Peeks next item in the buffer
		/** @return next item from the buffer, but leaves it inside the buffer. Next getNext will return it
		 * again. Automatically loads items, if buffer is empty */		 
		const ItemT &peek() const;
		
		///Peeks n-th item in the buffer
		/** Function retrieves value of n-th item in the buffer. If
		 *    item is not in the buffer, function tries to load it. If
		 *    loading is impossible (buffer is full), function 
		 *    throw exception IteratorNoMoreObjects. You can check
		 *    count of items in the buffer by calling function
		 *    getBufferRemain()
		 * 
		 *  @param n zero based index of item in the buffer from current position
		 *  @return item at position
		 *  @exception IteratoNoMoreObjects index is too high
		 *  @note function can block, if it have to wait for the data
		 */
        const ItemT &peek(natural n) const;
		
		///Tests, whether there are items
		/**
		 * @retval true there is at least one item to read
		 * @retval false there are no more items in the buffer, and also there are no more items to load
		 */
		bool hasItems() const;
		
		///Calculates total count of items can be read
		/**
		 * @return count of items as sum of items already loaded and items remaining in the input stream.
		 * If input stream returns unsupported, the same value is returned.
		 */
		natural getRemain() const;
		
		///Manually loads the buffer
		/**
		 * If there is empty space, it fills it with the items from the source stream. Otherwise, it
		 * silently returns
		 */ 		 
		void load() const;
		
		///Checks, whether buffer is empty
		/**
		 * @retval true buffer is empty, next call of peek() or getNext() will load the buffer
		 * @retval false buffer is not empty, next reading will be performed without loading
		 */
		bool isEmpty() const;
		
		///Returns item to the stream
		/**
		 * Function works similar to putback() function. It puts back the item, so it will be read
		 * by next calling of getNext() function. You don't need return the same item which has been
		 * read. You can also call this function multiple-times, if you know, that there is still
		 * space in the buffer released by previous reading
		 * 
		 * @param item item to unget
		 * @exception WriteIteratorNoSpace There is no more space in the buffer to store character
		 * 
		 * @see geyUngetCount
		 */ 
		void unget(const ItemT &item);
		
		
		///Returns item to the stream
		/**
		 * Function works similar to putback() function. It puts back the item, so it will be read
		 * by next calling of getNext() function. Function puts back last read item. You can also call 
		 * this function multiple-times, if you know, that there is still space in the buffer released 
		 * by previous reading
		 * 
		 * @exception WriteIteratorNoSpace There is no more space in the buffer to store character
		 * 
		 *  @see geyUngetCount
		 */
		void unget();
		
		///Returns count of items remaining to empty buffer
		/**
		 * @return count of items can be read without reloading the buffer
		 */
		natural getBufferRemain() const;
		
		///Returns count of items that can be "unget"
		/**
		 * @return count of items that can be "unget"
		 * @see unget
		 */
		natural getUngetCount() const {return rdpos;}
		
        RdIterator &nxChain() {return rdIter;}
        const RdIterator &nxChain() const {return rdIter;}


	protected:
		mutable AutoArray<ItemT, Allocator> buffArr;
		mutable RdIterator rdIter;
		mutable natural rdpos;
		mutable natural wrpos;
		mutable bool noItems;

	private:
		template<class X>
		natural readToBuff(FlatArrayRef<ItemT, X> buff, bool all = false) const {
			return rdIter.blockRead(buff,all);
		}
	};

	///Implements writing buffer
	/** Class implements buffer used to store data temporary before they are flushed by the block writing iterator
	 * You should use buffering, when you working with iterator optimized for writing items by blocks. This
	 * is case of all I/O iterators, such a file streams, network streams, etc
	 * 
	 * WriteBuffer is iterator, which contains array, that is flushed automatically when it is full. 	 
	 */
	template<class WrIterator, class Allocator = StdAlloc >
	class WriteBuffer: public WriteIteratorBase<typename DeRefType<WrIterator>::T::ItemT, WriteBuffer<WrIterator,Allocator> > {
    public:
		
		typedef typename DeRefType<WrIterator>::T::ItemT ItemT;

		///Initializes buffer
		/** @param wrIter output iterator optimized for block writing
		 *  @param count size of buffer in the items
		 */
		WriteBuffer(WrIterator wrIter, natural count)
			:buffArr(count)
			,wrIter(wrIter),rdpos(0),wrpos(0) {}
		
		///Destructor
		/** Destructor also flushes all items in the buffer. Because exceptions should not
		 * thrown in the destructor, you should call flush() explicitly, before destruction
		 * 
		 * @note Destructor ignores exception WriteIteratorNoSpace, because data in the buffer are already lost.
		 *  Other exceptions are reported through the fdthrow() function as  FwThrowAction::actError
		 * 
		 * @see flush
		 */
		~WriteBuffer() {
			try {
				flush();
			} catch (...) {
				if (!std::uncaught_exception())
				    throw;
			}
		}
		///Writes item into the buffer
		/**
		 * Writes item into the buffer. Flushes buffer, if it is necesery
		 */
		void write(const ItemT &x);
		///Manually flushes the buffer
		/**
		 * Writes _all_ items into the stream.
		 * 
		 * @note There is small difference between this function and flush performed during writing. The
		 * second one is called "fastflush" and perform only smallest necesery writing. This is useful
		 * if you writing into network stream and there is no enough space in internal outgoing buffer.
		 * Until buffer is transported, your program can continue to collect next items. See 
		 * more information on description of function __fastflush() 
		 * 
		 * This function performs "complete flush". After function is returned, buffer should be empty, all
		 * data should be transported to the output iterator
		 * 
		 * @exception WriteIteratorNoSpace during flush, iterator reaches end of write space. Remain of 
		 * 		buffer is probably lost
		 * @exception any Any I/O exception can be thrown
		 */
		void flush();
		
		///Test, whether there are space for items
		/**
		 * Function also test output stream for the space and allows to report "no space" even there is still
		 * free space in the buffer. It needs iterator, that reports valid remaining space
		 * 
		 * @retval true there is space to write items
		 * @retval false no more space to write items
		 */
		bool hasItems() const;
		
		///Calculates remain space
		/**
		 * It gets remain space of the output stream and substracts count of items already in the buffer.
		 * Result should be above or equal to zero and it is returned as result
		 */ 
		natural getRemain() const;
		
		///Tests, whether buffer is empty
		/**
		 * @retval true buffer is empty
		 * @retval false buffer is not empty (contains items)
		 */
		bool isEmpty() const;
		
		///Tests, whether buffer is full
		/**
		 * @retval true buffer is full, next write() causes flush of the buffer
		 * @retval false bffer is not full, there is still space to write items
		 */
		bool isFull() const;
		
		///Undoes writting
		/**
		 * Similar to putback and ReadBuffer::unget, you can rollback writting. 
		 * This is done by removing one item from the buffer. Removed item is
		 * returned as result
		 * 
		 * @return item removed from the stream
		 * @exception IteratorNoMoreObjects there are no more items that can be unwritten
		 */
		const ItemT &unwrite();
		
		///Calculates count of items can be written to the buffer without implicit flush
		/**
		 * @retval count of items to fill the buffer
		 */
		natural getBufferRemain() const;
		
		///Calculates count of items can be unwrite
		/**
		 * @return coun tof items to unwrite before exception is thrown
		 */
		natural getUnwriteCount() const {return wrpos - rdpos;}

        WrIterator &getChainedIterator() {return wrIter;}
        const WrIterator &getChainedIterator() const {return wrIter;}

    protected:
    	
    	///Performs fast flush
    	/** Fast flush only releases some space from the buffer. Function is always trying to
    	 * flush whole buffer, but blockWrite function can write less items, that has been
    	 * requested. __fastflush will not try to repeat writting
         *
         * @param nodatashift do not perform data shifting. Probably repeating of call will be performed 
         * 
		 * @exception WriteIteratorNoSpace during flush, iterator reaches end of write space. Remain of 
		 * 		buffer is probably lost
		 * @exception any Any I/O exception can be thrown
    	 */
		void __fastflush(bool nodatashift);
    	mutable AutoArray<ItemT, Allocator> buffArr;
    	mutable WrIterator wrIter;
    	mutable natural rdpos;
    	mutable natural wrpos;						
	};


	///Static buffer, whole allocated at stack
	/** Static read buffer is read buffer allocated at stack
	 * You must supply its size as template argument. Class
	 * is very fast for ad-hoc buffers needed to read small
	 * count of items */
	template<class RdIterator, natural count>
	class StReadBuffer: public ReadBuffer<RdIterator, StaticAlloc<count> >
	{
	public:
	    typedef ReadBuffer<RdIterator, StaticAlloc<count> > Super;
	    StReadBuffer(RdIterator iter):Super(iter,count) {}
	    
	};
	
    ///Static buffer, whole allocated at stack
    /** Static write buffer is write buffer allocated at stack
     * You must supply its size as template argument. Class
     * is very fast for ad-hoc buffers needed to write small
     * count of items */
    template<class WrIterator, natural count>
    class StWriteBuffer: public WriteBuffer<WrIterator, StaticAlloc<count> >
    {
    public:
        typedef WriteBuffer<WrIterator, StaticAlloc<count> > Super;
        StWriteBuffer(WrIterator iter):Super(iter,count) {}        
    };
    


	//-------------------------------------------------------------------------------
	
	template<class RdIterator, class Allocator>
	const typename ReadBuffer<RdIterator,Allocator>::ItemT &ReadBuffer<RdIterator,Allocator>::getNext() {
		if (isEmpty()) load();
		return buffArr[rdpos++];
	}

	template<class RdIterator, class Allocator>
	const typename ReadBuffer<RdIterator,Allocator>::ItemT &ReadBuffer<RdIterator,Allocator>::peek() const {
		if (isEmpty()) load();
		return buffArr[rdpos]; 
	}

	template<class RdIterator, class Allocator>
	bool ReadBuffer<RdIterator,Allocator>::hasItems() const {
		if (isEmpty()) {
			if (noItems) return false;
			try {
				load();
			} catch (IteratorNoMoreItems &) {
				noItems = true;
			}
		}
		return !isEmpty();
	}

	template<class RdIterator, class Allocator>
	natural ReadBuffer<RdIterator,Allocator>::getRemain() const {
		natural buffrm = wrpos - rdpos;
		return buffrm;
	}

	
	
	template<class RdIterator, class Allocator>
	void ReadBuffer<RdIterator,Allocator>::load() const {
		natural sp = buffArr.length() - wrpos;
		if (sp == 0) {
			if (rdpos == 0) return;
			natural x = 0;
			while (!isEmpty()) buffArr(x++) = buffArr[rdpos++];
			rdpos = 0;
			wrpos = x;
			return load();
		}
	

		try {
			natural wr = readToBuff(buffArr.tail(sp).expr());
			wrpos += wr;
			noItems = false;
		} catch (const IteratorNoMoreItems &) {
			if (isEmpty()) throw;
		}
	}
	template<class RdIterator, class Allocator>
	bool ReadBuffer<RdIterator,Allocator>::isEmpty() const {
		return rdpos>=wrpos;
	}

	template<class RdIterator, class Allocator>
	void ReadBuffer<RdIterator,Allocator>::unget(const ItemT &item) {
		if (rdpos>0) {
			buffArr(--rdpos) = item;			
		} else {
			throw WriteIteratorNoSpace(THISLOCATION,typeid(ItemT));
		}
	}

	template<class RdIterator, class Allocator>
	void ReadBuffer<RdIterator,Allocator>::unget() {
		if (rdpos>0) {
			--rdpos;			
		} else {
			throw WriteIteratorNoSpace(THISLOCATION,typeid(ItemT));
		}		
	}

	template<class WrIterator, class Allocator>
	void WriteBuffer<WrIterator,Allocator>::write(const ItemT &x) {
		if (isFull()) __fastflush(false);
		buffArr(wrpos++) = x;		
	}
	template<class WrIterator, class Allocator>
	void WriteBuffer<WrIterator,Allocator>::flush() {
		while (!isEmpty()) __fastflush(true);
		__fastflush(false);
	}

	template<class WrIterator, class Allocator>
	void WriteBuffer<WrIterator,Allocator>::__fastflush(bool nodatashift) {
	    if (isEmpty()) {
	        rdpos = wrpos = 0;
	    } else {
	        natural wr = wrIter.blockWrite(buffArr.mid(rdpos,wrpos - rdpos).expr());
	        rdpos+=wr;

	        if (rdpos == wrpos) {
	            rdpos = wrpos = 0;
	        } else if (nodatashift == false) {
	            natural x = 0;
	            while (rdpos < wrpos) {
	                buffArr(x++) = buffArr[rdpos++];
	            }
	            wrpos -= x;
	            rdpos = 0;
	        }
	    }
	}
	template<class WrIterator, class Allocator>
	bool WriteBuffer<WrIterator,Allocator>::hasItems() const {
		return wrIter.hasItems() && getRemain() > 0;
	}
	template<class WrIterator, class Allocator>
	natural WriteBuffer<WrIterator,Allocator>::getRemain() const {
		natural rm = wrIter.getRemain();
		if (rm < (wrpos - rdpos)) return 0;
		return rm - (wrpos - rdpos);
	}
	
	template<class WrIterator, class Allocator>
	bool WriteBuffer<WrIterator,Allocator>::isEmpty() const {
		return rdpos >= wrpos;
	}

	template<class WrIterator, class Allocator>
	bool WriteBuffer<WrIterator,Allocator>::isFull() const {
		return wrpos >= buffArr.length();
	}
	template<class WrIterator, class Allocator>
	const typename WriteBuffer<WrIterator,Allocator>::ItemT &WriteBuffer<WrIterator,Allocator>::unwrite() {
		if (wrpos > rdpos) {
			return buffArr[--wrpos];
		} else {
			throw IteratorNoMoreItems(THISLOCATION,typeid(ItemT));
		}
	}

	template<class RdIterator, class Allocator>
	natural ReadBuffer<RdIterator,Allocator>::getBufferRemain() const {
		return wrpos - rdpos;
	}
	
	template<class WrIterator, class Allocator>
	natural WriteBuffer<WrIterator,Allocator>::getBufferRemain() const {
		return buffArr.size() - wrpos;
	}

    template<class RdIterator, class Allocator>
    const typename ReadBuffer<RdIterator,Allocator>::ItemT &
            ReadBuffer<RdIterator,Allocator>::peek(natural n) const {
        
        if (n >= buffArr.size()) 
            throw IteratorNoMoreItems(THISLOCATION,typeid(ItemT));
        while (n >= getBufferRemain()) {
            load();
        }
        return buffArr.at(n);        
    }

}


#endif /*BUFFER_H_*/
