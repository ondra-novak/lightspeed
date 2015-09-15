#ifndef LIGHTSPEED_CONTAINERS_AUTOARRAY_H_
#define LIGHTSPEED_CONTAINERS_AUTOARRAY_H_

#include <utility>
#include "flatArray.h"
#include "../memory/stdAlloc.h"
#include "move.h"
#include "../memory/sharedResource.h"

namespace LightSpeed {

    
    template<class T, class Alloc = StdAlloc >
    class AutoArrayT: public FlatArrayBase<T, AutoArrayT<T,Alloc> >  {
    public:
        typedef typename Alloc::template AllocatedMemory<T> AllocatedMemory;
        typedef T ItemT;
        typedef AutoArrayT InitT;
        typedef FlatArrayBase<T, AutoArrayT<T,Alloc> > Super;

		typedef typename Super::OrgItemT OrgItemT;
		typedef typename Super::ConstItemT ConstItemT;
		typedef typename Super::NoConstItemT NoConstItemT;

        ///Creates empty array
        AutoArrayT();

        ///Creates empty array
        AutoArrayT(const InitT &init);

        AutoArrayT(const Alloc &alloc):allocBk(alloc,0),usedSz(0) {}

        
        ~AutoArrayT();

        ///assignment operator
        AutoArrayT &operator=(const AutoArrayT &arr);

        ///Reserves memory for extra items
        /**
         * @param items count of items that will be added soon. Function prepares
         * space for these items
         */
        void addExtra(natural items);
        
        ///Removes memory reserved for extra items
        /**
         * Function truncates memory block. It makes relocation, if it is necesery. Note that
         * you don't need to call this function, if you know, that a few extra memory 
         * is not important, because function can cost little performance
         */
        void freeExtra();
        
        ///Reserves memory for given items
        /**
         * @param items total count of items required to be in the array. If value
         * is less, than current count of items, function silently do nothing.
         * 
         * Function will never truncate already reserved block. It check,
         * whether requiread memory is already reserved, or tries
         * reserve it, if it is necesery
         */
        void reserve(natural items);
        

        ///Adds new item
        /**
         * @param item item to add. Item is added at the end of array (like std::vector::push_back)
         */
        void add(const T &item);
        
        ///Adds items
        /**
         * @param item item to add
         * @param count count of repeats
         */
        void add(const T &item, natural count);
        
        ///Appends array
        template<typename A, typename B>
        void append(const ArrayT<A,B> &arr);

        template<typename A, typename B>
        void append(IIterator<A,B> &arr);

        template<typename A, typename B>
        void append(IIterator<A,B> &arr, natural limit);

        
        ///Changes count of items
        /**
         * @param items count of items reqired
         * @param init value of item used to add, if adding new items leads to the target size 
         * 
         * function adds or removes items to get target size
         */
        void resize(natural items, const T &init);

        void resize(natural items);

        
        ////Inserts item at position
        /**
         * @param at position
         * @param init value to insert to the position
         * @exception RangeException thrown if at is out of range
         * 
         * Function moves all items from position to the end of array and creates new item
         * at the position. Inserting is not optimized, if you need to insert more items,
         * you should use another container
         */
        void insert(natural at, const T &init);
        ////Inserts count items at position
        /**
         * @param at position
         * @param init value to insert to the position
         * @param count count of items to insert
         * @exception RangeException thrown if at is out of range
         */
        void insert(natural at, const T &init, natural count);

		template<typename A, typename B>
		void insertArray(natural at, const ArrayT<A,B> &items);

        ///Erases item at position
        /**
         * @param at position of item to remove
         * @exception RangeException thrown if at is out of range
         * 
         * Function moves all items between position and the end of array. This can be
         * slow for large arrays 
         */
        void erase(natural at);

        ///Erases item at position referenced by the iterator
        void erase(const typename Iterator &iter);
		///Erases items at position
        /**
         * @param at position of item to remove
         * @param count count of items. If at + count is above size, count is corrected
         * @exception RangeException thrown if at is out of range
         * 
         * Function moves all items between position and the end of array. This can be
         * slow for large arrays 
         */
        void erase(natural at, natural count);
        
        ///Erases last 'count' items
        /**
         * @param count count of items to erase
         */
        void trunc(natural count = 1);
        
        ///Empties array
        /**
         * Removes all items. It keeps memory reserved. To remove items and memory, call clear() and freeExtra()
         */
        void clear();
        
        ///Write iterator
        /**
         * @see getWriteIterator
         */
        class WriteIter: public WriteIteratorBase<T, WriteIter> {
        public:
            WriteIter(AutoArrayT &target)
                :limit(naturalNull),target(target) {}
            WriteIter(AutoArrayT &target, natural limit)
                :limit(limit),target(target) {
                target.addExtra(limit);
            }
            
            bool hasItems() const {
                return limit > 0;
            }
            
            void write(const T &item) {
                if (hasItems()) {
                   target.add(item);
                   --limit;
                } else {
                    throwWriteIteratorNoSpace(THISLOCATION,typeid(T));
                }
            }
                                                
        protected:
            natural limit;
            AutoArrayT &target;
            
            
        };
        
        ///Insert iterator
        /**
         * @see getInsertIterator
         */
        class WriteInsertIter: public WriteIteratorBase<T, WriteInsertIter>, public SharedResource {
        public:
            WriteInsertIter(AutoArrayT &target, natural atpos, natural limit)
            :atpos(atpos),limit(limit),target(target) {
                target.reserveAt(atpos,limit);
            }
            
            ~WriteInsertIter() {
            	if (!isShared()) {
					try {
						if (limit) target.unreserveAt(atpos,limit);
					} catch (...) {
						if (!std::uncaught_exception())
							throw;
					}
            	}
            }
            
            bool hasItems() const {
                return limit > 0;
            }
            
            void write(const T &item) {
                if (hasItems()) {
                    new(target.data() + atpos) T(item);
					atpos++;
					limit--;
                } else {
                    throwWriteIteratorNoSpace(THISLOCATION, typeid(T));
                }
            }
            
        protected:
            natural atpos;
            natural limit;
            AutoArrayT &target;
        };
        

        ///Retrieves write iterator
        /**
         * @return iterator, that allows to write into the array. Each write is calling of function add()
         */
        WriteIter getWriteIterator() {
        	return WriteIter(*this);
        }

        ///Retrieves write iterator
        /**
         * @param limit count of items will be written at maximum. Function optimizes memory allocation
         * to receive specified count in optimal performance
         * @return iterator, that allows to write into the array. Each write is calling of function add()
         */
        WriteIter getWriteIterator(natural limit) {
        	return WriteIter(*this, limit);
        }

        
        ///Retrieves insert iterator
        /**
         * @param at position to where data should be inserted
         * @param count count of items to be added
         * @return iterator that should be used to writing. Note, this iterator is not compatible with
         * iterator returned by getWriteIterator() function. This because, iterator can finally receive 
         * less items, than required and have to reclaim remaining revesed space
         */
        WriteInsertIter getInsertIterator(natural at, natural count) {
        	if (at > usedSz) 
        		throwRangeException_FromTo<natural>(THISLOCATION,0,usedSz,at);
        	return WriteInsertIter(*this,at,count);
        
        }
        
        
        
        T *data() {return allocBk.getBase();}
        
        const T *data() const {return allocBk.getBase();}
        
		OrgItemT *refData(NoConstItemT *) const {return const_cast<OrgItemT *>(data());}
		const OrgItemT *refData(ConstItemT *) const {return data();}


        natural length() const {return usedSz;}
        
        natural getAllocatedSize() const {return allocBk.getSize();}
        

        void swap(AutoArrayT &arr);

        Alloc getAllocator() const {return Alloc(allocBk);}

    protected:
        AllocatedMemory allocBk;
        natural usedSz;
        

        

        void reserveAt(natural pos, natural count);
        void unreserveAt(natural pos, natural count);

    };

    
    template<class T, class Alloc = StdAlloc >
    class AutoArray: public AutoArrayT<T,Alloc> {
    public:
    	typedef AutoArrayT<T,Alloc> Super;
        AutoArray() {}

        explicit AutoArray(const Alloc &alloc):Super(alloc) {}

        ///Creates array and initializes it to count of items
        /**
         * @param count count of items to have the array
         * @param init value used to initialize the items 
         */
        explicit AutoArray(natural count, const T &init = T(), const Alloc &alloc= Alloc());
        
        ///Creates array and initializes it using reading iterator
        /**
         * @param iter iterator used to read the data
         * 
         * @note Function uses getRemain() function to retrieve count of items to
         * prereserve memory. 
         */
        template<class I, class X>
        explicit AutoArray(IIterator<I,X> &iter, const Alloc &alloc = Alloc()):Super(alloc) {
        	Super::append(iter);
        }
        
        ///Creates array and uses GenericArray to initialize               
        template<class A,typename B>
        explicit AutoArray(const ArrayT<A,B> &arr, const Alloc &alloc = Alloc()):Super(alloc) {
        	Super::append(arr);
        }

        
        ///assignment operator
        AutoArray &operator=(const AutoArray &arr) {
        	Super::operator =(arr);
        	return *this;
        }

     //   AutoArray(MoveConstruct,AutoArray &obj);

    protected:

    };


    template<typename T, typename Alloc = StdAlloc>
    class AutoArrayStream: public AutoArray<T,Alloc>::WriteIter {
    public:
    	AutoArrayStream():AutoArray<T,Alloc>::WriteIter(buff) {}
    	AutoArrayStream(const Alloc &a):AutoArray<T,Alloc>::WriteIter(buff),buff(a) {}
    	AutoArrayStream(const AutoArray<T,Alloc> &a):AutoArray<T,Alloc>::WriteIter(buff),buff(a) {}
    	AutoArrayStream(const AutoArrayStream &other)
			:AutoArray<T,Alloc>::WriteIter(buff),buff(other.buff) {}
    	natural length() const {return buff.length();}
    	bool empty() const {return buff.empty();}
    	const T *data() const {return buff.data();}
    	typedef typename AutoArray<T,Alloc>::Iterator Iterator;
    	Iterator getFwIter() const {return buff.getFwIter();}

    	const AutoArray<T,Alloc> &getArray() const {return buff;}
    	AutoArray<T,Alloc> &getArray() {return buff;}
    	void clear() {buff.clear();this->limit = naturalNull;}


    	typedef AutoArray<T,Alloc> ArrayType;
    protected:
    	AutoArray<T,Alloc> buff;
    };
   


    template<class T, class Alloc>
    AutoArray<T,Alloc> moveConstruct(AutoArray<T,Alloc> &obj) {
        return AutoArray<T,Alloc>(_moveConstruct, obj);
    }


    
    template<typename A,typename B>
    class MoveObject<AutoArray<A,B> >: public MoveObject_Construct {};

} // namespace LightSpeed


namespace std {
    template<typename T,  typename Alloc>
    inline void swap(::LightSpeed::AutoArrayT<T,Alloc> &__lhs,
                     ::LightSpeed::AutoArrayT<T,Alloc> &__rhs) {
        __lhs.swap(__rhs);
    }
}



#endif /*AUTOARRAY_H_*/
