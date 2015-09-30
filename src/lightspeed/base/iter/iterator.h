#ifndef LIGHTSPEED_ITERATOR_H_
#define LIGHTSPEED_ITERATOR_H_

#include <typeinfo>
#include "../types.h"
#include "../qualifier.h"
#include "../compare.h"
#include "../qualifier.h"
#include "../exceptions/throws.h"


namespace LightSpeed
{


    template<typename T, class Traits> class FlatArray;
    template<typename T, class Traits> class FlatArrayRef;
	
	
	
	///Common class for all iterators
    template<class Impl>
    class IIteratorCommon: public ComparableLessAndEqual<Impl> {
    public:
        static const natural unsupported = naturalNull; 


        ///Tests, whether iterator can return more items
        /**
         * @retval true Iterator contains at least one item. Function
         *      getNext() will return this item
         * @retval false Iterator is empty, no more items. Function
         *      getNext() will throw an exception
         *
         * @note Function is available also for IWriteIterator. Function
         *    returns true, if there is space to write next object
         */
        bool hasItems() const {
            return this->_invoke().hasItems();
        }
        
        ///Retrieves remain count of items ready
        /**
         * @return count of items ready for reading. It can be
         * count of items really available to the end of file,
         * or count of items that can be read without blocking.
         * It depends on type of contained. Function returns unsupported
         * which is equal to infinite, when function is not supported
         * and hasItems returns true. Otherwise, it returns 0
         */
        natural getRemain() const {
            return this->_invoke().getRemain();
        }
	
        ///Tests, whether two iterators contains (or points to) the same item
        /**
         * @param other other iterator
         * @retval true iterators are the same
         * @retval false iterator are not the same
         * @note Implementer should implement this function to allow operators == and != to
         * work. You should not call this function directly, it is better to use standard
         * operators == and !=
         * @exception UnsupporedFeature operator cannot be compared
         * @exception IncompatibleType other operator is not compatible with the object
         */
        bool equalTo(const Impl &other) const {
                return this->_invoke().equalTo(other);
        }
    
        ///Tests, whether current iterator is less then other iterator
        /**
         * @param other other iterator
         * @retval true current iterator is less
         * @retval false current iterator is not less
         * @note Implementer should implement this function to allow compare operators to
         * work. If you implement thisYou should not call this function directly, it is better to use standard
         * operators == and !=
         * @exception UnsupporedFeature operator cannot be compared
         * @exception IncompatibleType other operator is not compatible with the object
         */
        bool lessThan(const Impl &other) const {
                return this->_invoke().lessThan(other);
        }
    };
    
  

    
    
    ///Standard read iterator interface
    /** This is a template interface. You should use the interface to
     * declare function parameter to assert class that is really iterator.
     * 
     * Don't extend this class directly. You should extend IteratorBase class
     * 
     * @see IteratorBase
     */
    template<class T, class Impl>
    class IIterator: public IIteratorCommon<Impl> {
        
    public:     
        

        typedef T ItemT;
        
        ///Retrieves next item
        /**
         * @return item retrieved from iterator. item is removed or
         *  marked as retrieved and iterator will never return it again
         * @exception IteratorNoMoreItems iterator reaches end of
         *  items and cannot retrieve anything
         * 
         * @note Function returns result as reference. This technique
          is used to speed up iterator's performance, because most of
          containers can allow to access objects directly, without 
          copying. Returned value is also const, so you are unable to
          modify the objects. Note that there can be containers, that
          cannot retrieve items as references. It is possible, that
          result will be reference to one variable allocated internally in
          the iterator's content and every getNext causes, that content
          of the value will be overwritten. You should never convert
          returned reference to pointer and store these pointers. Returned
          reference is guaranteed to be valid until next getNext() or 
          peek() is called
          */

          
        const T &getNext() {
                return this->_invoke().getNext();
        }

        
        ///Retrieves next item, but will not remove/mark it
        /**
         * Allows to look ahead, what item is ready in the 
         * container.
         * @return  item that is ready to retrieve
         * @exception IteratorNoMoreItems iterator reaches end of
         *  items and
         * @exception UnsupportedFeature whether iterator doesn't
         *  support this function
         *
         * @note Function returns result as reference. This technique
         is used to speed up iterator's performance, because most of
         containers can allow to access objects directly, without 
         copying. Returned value is also const, so you are unable to
         modify the objects. Note that there can be containers, that
         cannot retrieve items as references. It is possible, that
         result will be reference to one variable allocated internally in
         the iterator's content and every getNext causes, that content
         of the value will be overwritten. You should never convert
         returned references to pointers and store these pointers. Returned
         reference is guaranteed to be valid until next getNext() or 
         peek() is called
         *  
         */
        
        const T &peek() const {
                return this->_invoke().peek();
        }
        
        ///perform block reading
        /** 
         * Block reading is optional feature for the iterator. Every iterator
         * can transfer data to the prepared generic array. This feature
         * is designed to implementing reading buffers. Iterator
         * can overwrite this function and implement faster data exchange
         * 
         * @param buffer buffer as generic array. Function overwrites item
         *  in the buffer using function set(). It will fills up to size of
         *  array. If array is empty, function do nothing.
		 * @param readAll when set to true, function will try to fill whole
		 *  buffer. If iterator is network stream, it can cause, that function
		 *  will wait, until all data arrives. End of stream reached during
		 *  readAll is true causes exception. When set to false, function
		 *  guarantees that at least one item will be read (causing blocking 
		 *  wait when there are no items ready). End of stream reached during
		 *  readAll is false is not exception, but zero is returned
         *
         * @return count count of items read from stream to buffer. Function
		 * will return zero, when buffer is empty. Function also returns zero,
		 * when readAll is false and there are no more items (stream is closed)
		 * Function returns size of buffer when readAll is true
		 *
         * @exception IteratorNoMoreObjects should be thrown, if function
         *  reaches end of stream when readAll is true
         * @exception ...other... if exception is thrown during reading
         *  function will rethrow it only when no items read. Otherwise
         *  it exits reading and returns read items.
         *          
         */
        template<class Traits>
        natural blockRead(FlatArrayRef<T,Traits> buffer, bool readAll) {
            return this->_invoke().blockRead(buffer,readAll);
        }
        

        ///Skips one item
        /**
         * It is better to call skip() instead of calling getNext() without
         * getting return, because function can be optimized. Call skip()
         * if you don't need to know value of next item, or if you already
         * know it (for example, using peek())
         */
        void skip(){
             this->_invoke().skip();
        }
    };


    ///Interface for iterator that returns non-const references. 
    /** It allows to modify items in collections.
     *  
     * Don't extend this class directly. You should extend MutableIteratorBase class
     * 
     * @see MutableIteratorBase
     */
    template<class T, class Impl >
    class IMutableIterator: public IIterator<T,Impl> {
    public:

        ///Retrieves next item as rewritable reference
        /**
          @return reference to item. You can change value of item, and this value
            will be written into the container
            */
        T &getNextMutable() {
                return this->_invoke().getNextMutable();
        }
        
        T &peekMutable() const {
                return this->_invoke().peekMutable();
        }
    };


    ///Write iterator
    /** Write iterator is special iterator witch is designed
     * to perform writing instead reading. This is useful to
     * fill containers with a data or for example 
     * writting data into disk file
     * 
     * Don't extend this class directly. You should extend WriteIteratorBase class
     * 
     * @see WriteIteratorBase

     */
    template<class T, class Impl >
    class IWriteIterator:public IIteratorCommon<Impl> {
        
    public:
        typedef T ItemT;
        
        ///Writes into container
        /**
         * @param x item to write
         * @exception WriteIteratprNoSpace iterator cannot write
         *  because there is no space for new item
         */
        void write(const T &x) {
                 this->_invoke().write(x);
        }

        ///Asks write iterator, it it can accept following item
        /** @param x item to test
         * @retval true item is accepted and can be written
         * @retval false item is not accepted, writing causes exception
         */
        bool canAccept(const T &x) const {
            return this->_invoke().canAccept(x);
        }

        ///Writes data read from iterator up to limit
        /**
         * @param input input iterator that contains data
         * @param limit count of items to copy, default means that reading
         *      will stop on end of file
         * @return count of items copied
         */
        template<class RdIterator>
        natural copy(RdIterator input,natural limit = naturalNull) {
                return this->_invoke().copy(input,limit);

        }
        ///Writes data read from the iterator up to limit or until reach first unacceptable item
        /**
         * @param input input iterator that contains data. It need to support peek() operation
         * @param limit count of items to copy, default means that reading
         *      will stop on end of file         *
         * @return count of items copied
         *
         * @note write iterator have to support canAccept function. If item is
         * not accepted, it is not copied and copying stops
         */
        template<class RdIterator>
        natural copyAccepted(RdIterator input,natural limit = naturalNull) {
                return this->_invoke().copyCanAccept(input,limit);
        }
        
        ///Performs block writing to the output stream
        /**
         * Block writing can be fast, when primitive objects are written and
         * iterator contains optimization for this. This includes all
         * disk I/O streams writing bytes at one cycle.
         *
         * @param buffer reference to flat array contains data to write
         * @param writeAll when true, iterator tries to write everything, even
         *   if it requires waiting to complettion. If false used, iterator
         *   writes faster as possible, without waiting which may cause
         *   that less bytes will be written. Note that function always writes
         *   at least one item and it will always wait for complettion of
         *   previous writes, when there is no left space for at least one
         *   item in the output buffer
         *
         * @return count of items written, equal to length of array when
         *   writeAll is set.
         *
         * @exception WriteIteratorNoSpace there is no space to write at least
         * one item. When writeAll is true, function throws exception when
         * it cannot carry to write all. But already written items are not
         * removed.
         */
        template<class Traits>
        natural blockWrite(const FlatArray<typename ConstObject<T>::Remove,Traits> &buffer, bool writeAll = true) {
                return this->_invoke().blockWrite(buffer, writeAll);
        }
        template<class Traits>
        natural blockWrite(const FlatArray<typename ConstObject<T>::Add,Traits> &buffer, bool writeAll = true) {
                return this->_invoke().blockWrite(buffer, writeAll);
        }

		///Copies data from read iterator to write iterator using block operations
		/**
		 * Useful to copy streams optimized for block operations, such
		 * a files. There is no speedup for streams not supporting block operations
		 *
		 * @param input source of data
		 * @param buffer used to store data temporary. Caller must allocate
		 *   some and function tries to use whole
		 * @param limit count of bytes to read, default value means unlimited
		 * @retval count of items copied.
		 *
		 * Copying stops, when limit count items has been processed. Function
		 * uses readAll=false for reading and writeAll=true for writing,
		 * so network streams don't need to use whole buffer, function 
		 * tries to write all data immediately when they arrives
		 *
		 *
		 */
		 
		template<typename RdIterator, typename Traits>
		natural blockCopy(RdIterator &input, FlatArray<T,Traits> &buffer, natural limit = naturalNull) {
			return this->_invoke().blockCopy(input,buffer,limit);
		}

        ///causes any internal buffer to flush
        /**
         * Every IWriteIterator can has some internal buffer to handle its features.
         * By default, all buffers should be flushed in the destructor of the iterator
         * Unfortunately, if exception is thrown in the destructor, it will not probably
         * thrown out, and will be ignored. You will not notified about failure.
         *
         * Function flush() causes, that internal buffers will be flushed now, and  any
         * potential exception can be thrown during the flush and you are able to catch it
         *
         * After flush returns, all data from any internal buffers should be
         * written into the target, if this is possible. Function also
         * can block execution during waiting for finishing operation. Responsibility
         * to flush external buffers is given to the OS, but if there is
         * way, how to flush them, target iterator also should handle it.
         */
        void flush() {
        	 this->_invoke().flush();
        }

        ///Skips next item writing its default value to the stream
        /**
         * You can use this function to write empty item. Function can
         * be optimized. But it cannot be used, if class of items
         * doesn't define default constructor.
         */
        void skip(){
             this->_invoke().skip();
        }

        ///Prepares iterator - stream, that specified count of items will be written
        /** Reserves space in output stream. This can help iterator
         * to better work with internal memory. Function is hint, not all iterators
         * can implement this. Iterator doesn't neet always relay on the parameter
         *
         * @param items count of items will be written. If count of written
         * items is less than requested, iterator can waste a memory. If count
         * of written item is greater than requested, iterator will propably need
         * to remanage memory of internal buffers and result will be less performance
         */
        void reserve(natural itemCount) {
        	 this->_invoke().reserve(itemCount);
        }
};

    
    
    ///Declaration of expendable iterator class
    /**
     * You should always extend this class, instead directly extension
     * of IIterator, because this class contains implementation
     * for all unimplemented methods and prevents possible
     * recursive infinite loop, that can be made, if there is
     * unimplemented method in combination of direct extending.
     * 
     * 
     * @see IIterator
     */
    
    template<class T, class Impl>
    class IteratorBase: public IIterator<T, Impl> {
        
    public:		
        
        friend class IIterator<T, Impl>;
        
        
        bool hasItems() const;
        natural getRemain() const {
            return this->_invoke().hasItems()?this->unsupported:0;
        }
        const T &getNext();
        const T &peek() const {
                throwUnsupportedFeature(THISLOCATION,&this->_invoke(),"peek()");
                throw;
        }
        
        template<class Traits>
        natural blockRead(FlatArrayRef<T, Traits> buffer, bool readAll) {
    		natural rd = 0;
    		natural sz = buffer.length();
    		if (sz == 0) return 0;
        	
    		while (this->_invoke().hasItems() && rd < sz) {
    			try {
    				buffer.set(rd,this->_invoke().getNext());
    			} catch (...) {
    				if (rd == 0 || readAll) throw;
    				break;
    			}
    			++rd;
    		}
    		if (readAll && rd !=sz) 
    			throwIteratorNoMoreItems(THISLOCATION,typeid(T));
        	
    		return rd;
        }
        bool equalTo(const Impl &) const {
            throwUnsupportedFeature(THISLOCATION,&this->_invoke(),"equalTo()");
            throw;
        }
        
        bool lessThan(const Impl &) const {
            throwUnsupportedFeature(THISLOCATION,&this->_invoke(),"lessThan()");
            throw;
        }        
        void skip(){
            this->_invoke().getNext();
        }

    };

    
    ///Declaration of extendable mutable iterator class
    /**
     * You should always extend this class, instead directly extension
     * of IMutableIterator, because this class contains implementation
     * for all unimplemneted methods and prevents possible
     * recursive infinite loop, that can be made, if there is
     * unimplemented method in combination of direct extending.
     * 
     * Note, all members are protected. That because you cannot use this
     * class for accessing instance. You should use IIterator to accessing
     * the iterator's instance
     * 
     * @see IIterator
     */
    template<class T, class Impl>
    class MutableIteratorBase: public IteratorBase<T,Impl> {
    public:

        friend class IMutableIterator<T,Impl>;
        
        T &getNextMutable();

        T &peekMutable() const {
            throwUnsupportedFeature(THISLOCATION,&this->_invoke(),"peekMutable()");
            throw;
        }
    };

    
    ///Declaration of extendable write iterator class
    /**
     * You should always extend this class, instead directly extension
     * of IMutableIterator, because this class contains implementation
     * for all unimplemneted methods and prevents possible
     * recursive infinite loop, that can be made, if there is
     * unimplemented method in combination of direct extending.
     * 
     * Note, all members are protected. That because you cannot use this
     * class for accessing instance. You should use IWriteIterator to accessing
     * the iterator's instance
     * 
     * @see IWriteIterator
     */
    
    template<class T, class Impl>
    class WriteIteratorBase:public IWriteIterator<T, Impl> {
        
    public:
        friend class IWriteIterator<T, Impl>;
		void write(const T &);
	    bool canAccept(const T &) const {
	        //most common iterators accepts all items, so it only tests hasItems()
	        return this->_invoke().hasItems();
	    }
        
        template<class RdIterator>
        natural copy(RdIterator &input,natural limit = naturalNull) {
			natural count = 0;
			while (count<limit && input.hasItems() && this->_invoke().hasItems() ) {
				this->_invoke().write(input.getNext()); 
				count++;
			}
			return count;
        }
        template<class RdIterator>
        natural copy(const RdIterator &input,natural limit = naturalNull) {
        	RdIterator inputcpy(input);
        	return copy(inputcpy,limit);
        }
		template<typename RdIterator, typename Traits>
		natural blockCopy(RdIterator &input, FlatArray<T,Traits> &buffer, natural limit = naturalNull) {
			natural count = 0;
			natural bufsize = buffer.length();
			while (limit) {
				natural toread = bufsize < limit?bufsize:limit;								
				natural rd = input.blockRead(buffer.head(toread).ref(),false);
				if (rd == 0) return count;
				this->_invoke().blockWrite(buffer.head(rd),true);
				if (limit != naturalNull) limit-=rd;
			}
			return count;
		}
		template<class RdIterator>
        natural copyAccepted(RdIterator &input,natural limit = naturalNull) {
            natural count = 0;
            while (count<limit && input.hasItems()
                    && this->_invoke().canAccept(input.peek()) ) {
                this->_invoke().write(input.getNext());
                count++;
            }
            return count;
        }
        
        template<class Traits>
        natural blockWrite(const FlatArray<typename ConstObject<T>::Remove,Traits> &buffer, bool writeAll = true) {
    		natural wr = 0;
    		natural sz = buffer.length();
    		if (sz == 0) return 0;

			while (this->_invoke().hasItems() && wr < sz) {
				this->_invoke().write(buffer[wr]);
				wr++;
			}
    		if (wr != sz && writeAll)
    		    throwWriteIteratorNoSpace(THISLOCATION,typeid(T));
    		return wr;
		}

        template<class Traits>
        natural blockWrite(const FlatArray<typename ConstObject<T>::Add,Traits> &buffer, bool writeAll = true) {
    		natural wr = 0;
    		natural sz = buffer.length();
    		if (sz == 0) return 0;
        	
    		while (this->_invoke().hasItems() && wr < sz) {
    				this->_invoke().write(buffer[wr]);
                wr++;
    		}
    		if (wr != sz && writeAll)
    		    throwWriteIteratorNoSpace(THISLOCATION,typeid(T));
    		return wr;
		}

        void flush() {}
        void skip() {
            this->_invoke().write(T());
        }

        bool hasItems() const;
        natural getRemain() const {
            return this->_invoke().hasItems()?naturalNull:0;
        }
        void reserve(natural /*itemCount*/) {}

    };


    ///Random access iterator is able to process item in random order
    /**
     * @param BasicIterator specifies iterator to used as base iterator
     *      it can be any of following set (IIterator, IWriteIterator, IMutableIterator)
     *      or any other, which need to be extended with this interface. 
     *      Note, iterator includes basic set of constructors, so
     *      combinations of usage are limited     
     */
    template<class BasicIterator>
    class IRandomAccessIterator: public BasicIterator {
    public:
        typedef BasicIterator Super;
        typedef typename BasicIterator::ItemT ItemT;
        
        ///Seeks in the stream
        /** Random access iterator is able to seek on iterator forward
         * or back, or specify absolute position 
         * 
         * 
         * @param offset specifies offset to seek. You cannot use
         *   negative value
         * @param direction Specifies direction from namespace Direction. 
         *   Iterator should support directions 'forward', 'backward', 
         *   'absolute','nowhere'. Other directions are optional.
         *   if iterator cannot handle required direction, it can throw the
         *  exception UnsupportedFeature. Support for thiese directions
         *  is recomended, but not mandatory. Linked lists will probably not
         *  support absolute seeking. Default implementation also
         *  doesn't support backward, because feature is emulated by
         *  moving pointer through function getNext()
         * @return count of steps has been moved. Iterator can stop
         *  move sooner, when it reaches end of working area. seeking
         *  out of range is not an error.
         */
        
        
        natural seek(natural offset, Direction::Type direction) {
        	return this->_invoke().seek(offset,direction);
        }
        ///Retrieves current position
        /** Retrieves position of iterator, if this is supported
         * @return position, or naturalNull
         */
        natural tell() const {
            return this->_invoke().tell();
        }
        
        ///Tests, whether iterator can seek in required direction
        bool canSeek(Direction::Type direction) const {
        	return this->_invoke().canSeek(direction);
        }
               
        
        ///Calculates distance between two iterators
        /**
         * Default implementation requires tell() function working
         * otherwise, function must be redefined by derived class
         * 
         * @param other other iterator.
         * @retval function calculates count of items between this
         * iterator and other iterator, including current. If function
         * returns 0, both iterators should point at the same item
         * if function returns naturalNull, function is unable to
         * determine distance, or it is unsupported
         */
        natural distance(const IRandomAccessIterator &other) const {
        	return this->distance(other);
        }

        
        ///Calculates direction between two iterators
        /**
         * 
         * @param other other iterator.
         * @retval Direction::forward iterator must seek forward to reach other iterator
         * @retval Direction::backward iterator must seek backward to reach other iterator
         * @retval Direction::undefined direction is unknown
         * @retval Direction::nowhere iterators staying at the same position         
         */
        Direction::Type direction(const IRandomAccessIterator &other) const {
        	return this->_invoke().direction(other);
        }
        
        IRandomAccessIterator() {}
        
        IRandomAccessIterator(const BasicIterator &other):BasicIterator(other) {}

        IRandomAccessIterator(const IRandomAccessIterator &other):BasicIterator(other) {}
        
        template<class T>
        IRandomAccessIterator(const T &other):BasicIterator(other) {}

        
    };
    
    
    template<class BasicIterator>
    class RandomAccessIterator: public IRandomAccessIterator<BasicIterator> {
    public:
        typedef IRandomAccessIterator<BasicIterator>  Super;
        typedef typename BasicIterator::ItemT ItemT;
        
        natural seek(natural offset, Direction::Type direction) {
            if (direction == Direction::forward) {
                natural cnt  = offset;
                while (cnt && Super::hasItems()) {
                    Super::skip();
                    cnt--;
                }
                return offset - cnt;
            } else if (direction == Direction::nowhere) {
                return 0;
            } else {
                throwUnsupportedFeature(THISLOCATION,&(this->_invoke()),
                        "seek-unsupported direction");
            }
        }
        natural tell() const {
            return naturalNull;
        }
        
        bool canSeek(Direction::Type direction) const {
            if (direction == Direction::nowhere)
                return true;
            if (direction == Direction::forward)
                return Super::hasItems();
            return false;
        }
               
        natural distance(const RandomAccessIterator &other) const {
            natural pos1 = this->_invoke().tell();
            natural pos2 = other._invoke().tell();
            if (pos1 == naturalNull || pos2 == naturalNull) 
                return naturalNull;
            if (pos1 > pos2) return pos1 - pos2;
            else return pos2 - pos1;
        }

        natural getRemain(const Direction::Type dir) const {
            if (dir == Direction::forward)
                return Super::_invoke().getRemain();
            else
                return naturalNull;
        }
        
        Direction::Type direction(const RandomAccessIterator &other) const {
            natural pos1 = this->_invoke().tell();
            natural pos2 = other._invoke().tell();
            if (pos1 == naturalNull || pos2 == naturalNull)
                return Direction::undefined;
            if (pos1 > pos2) return Direction::backward;
            else if (pos1 < pos2) return Direction::forward;            
            else return Direction::nowhere;
        }
        
        RandomAccessIterator() {}
        
        RandomAccessIterator(const BasicIterator &other):Super(other) {}

        RandomAccessIterator(const RandomAccessIterator &other):Super(other) {}
        
        template<class T>
        RandomAccessIterator(const T &other):Super(other) {}

        bool equalTo(const RandomAccessIterator &other) const {
            return direction(other) == Direction::nowhere;
        }

        bool lessThan(const RandomAccessIterator &other) const {
            return direction(other) == Direction::forward;
        }
        
    };

	template<typename MutableIter>
	class WriteToMutableIter: public WriteIteratorBase<
		typename DeRefType<MutableIter>::T::ItemT, WriteToMutableIter<MutableIter> > 
		
	{
	public:
		typedef typename DeRefType<MutableIter>::T::ItemT T;
		WriteToMutableIter(MutableIter iter):iter(iter) {}

		void write(const T &x) {
			T &z = iter.getNextMutable();
			z = x;
		}
		bool hasItems() const {
			return iter.hasItems();
		}

		natural getRemain() const {
			return iter.getRemain();
		}

		bool isLess(const WriteToMutableIter &other) const {
			return iter.isLess(other.iter);
		}

		bool equalTo(const WriteToMutableIter &other) const {
			return iter.equalTo(other.iter);
		}

	protected:
		MutableIter iter;
	};

} // namespace LightSpeed



#endif /*ITERATOR_H_*/
