#ifndef LIGHTSPEED_VTITERATOR_H_
#define LIGHTSPEED_VTITERATOR_H_

#include "iterator.h"

namespace LightSpeed {

    ///Interface of virtual iterator
    /** Virtual iterator is similar to IIterator, but every
     * method of IIterator is virtual. You can use it everywhere,
     * where you cannot use generic IIterator, because non-teplate
     * class is requested. This class only requires to specify type
     * of item that will ne processed by this iterator.
     */
    template<typename T>
    class IVtIterator: public IteratorBase<T, IVtIterator<T> > {
    public:
        
        typedef IteratorBase<T, IVtIterator<T> > Super;
        virtual bool hasItems() const = 0;
        virtual natural getRemain() const {return Super::getRemain();}
        virtual const T &getNext() = 0;
        virtual const T &peek() const {return Super::peek();}
        virtual natural blockRead(ArrayRef<T> buffer, bool readAll) {
            return Super::blockRead(buffer.ref(),readAll);
        }
        template<class Traits>
        natural blockRead(FlatArray<T,Traits> &buffer) {
            ArrayRef<T> arr(buffer.data(),buffer.size());
            return this->blockRead(arr);
        }
        virtual bool equalTo(const IVtIterator &other) const {
            return Super::equalTo(other);
        }
        virtual bool lessThan(const IVtIterator &other) const {
            return Super::lessThan(other);
        }
        virtual void skip() { Super::skip();}
    };

    ///Interface of virtual mutable iterator
    /** Virtual mutable iterator is similar to IMutableIterator, but every
     * method of IMutableIterator is virtual. You can use it everywhere,
     * where you cannot use generic IMutabkeIterator, because non-teplate
     * class is requested. This class only requires to specify type
     * of item that will ne processed by this iterator.
     */
    template<typename T>
    class IVtMutableIterator: public MutableIteratorBase<T, IVtMutableIterator<T> >
    {
    public:
        typedef MutableIteratorBase<T, IVtMutableIterator<T> > Super;

        virtual bool hasItems() const = 0;
        virtual natural getRemain() const {return Super::getRemain();}
        virtual const T &getNext() = 0;
        virtual const T &peek() const {return Super::peek();}
        virtual natural blockRead(ArrayRef<T> buffer, bool readAll) {
            return Super::blockRead(buffer.ref(),readAll);
        }
        template<class Traits>
        natural blockRead(FlatArray<T,Traits> &buffer) {
            ArrayRef<T> arr(buffer.data(),buffer.size());
            return this->blockRead(arr);
        }
        virtual bool equalTo(const IVtMutableIterator &other) const {
            return Super::equalTo(other);
        }
        virtual bool lessThan(const IVtMutableIterator &other) const {
            return Super::lessThan(other);
        }
        virtual void skip() { Super::skip();}
    
        virtual T &getNextMutable() = 0;

        virtual T &peekMutable() const {return Super::peekMutable();}
    };

    ///Interface of virtual iterator
    /** Virtual iterator is similar to IIterator, but every
     * method of IIterator is virtual. You can use it everywhere,
     * where you cannot use generic IIterator, because non-teplate
     * class is requested. This class only requires to specify type
     * of item that will ne processed by this iterator.
     */
    template<class T>
    class IVtWriteIterator:public WriteIteratorBase<T,IVtWriteIterator<T> > {
    public:
        typedef WriteIteratorBase<T,IVtWriteIterator<T> > Super;
    
        virtual void write(const T &x) = 0;
        virtual bool canAccept(const T &x) const { return Super::canAccept(x);}
        template<class Traits>
        natural blockWrite(const FlatArray<T,Traits> &buffer, bool writeAll = true) {
            return this->blockWrite(ArrayRef<const T>(buffer.data(),buffer.size()),writeAll);
        }
        virtual natural blockWrite(const ArrayRef<const T> &buffer, bool writeAll = true) {
            return Super::blockWrite(buffer,writeAll);
        }

        virtual void flush() {Super::flush();}
        virtual void skip() {Super::skip();}

        virtual bool hasItems() const = 0;
        virtual natural getRemain() const {return Super::getRemain();}
    };

    ///Iterator virtual wrapper
    /** Wraps any generic iterator into the virtual iterator.
     */
    template<class Iterator>
    class VtIterator:
                public IVtIterator<typename DeRefType<Iterator>::T::ItemT> {
    public:
        typedef IVtIterator<typename DeRefType<Iterator>::T::ItemT> Super;
        typedef typename Super::ItemT ItemT;
        VtIterator(Iterator iter):iter(iter) {}
        virtual bool hasItems() const {return iter.hasItems();}
        virtual natural getRemain() const {return iter.getRemain();}
        virtual const ItemT &getNext() {return iter.getNext();}
        virtual const ItemT &peek() const {return iter.peek();}
        virtual natural blockRead(ArrayRef<ItemT> buffer, bool readAll) {
            return iter.blockRead(buffer.ref(),readAll);
        }
        virtual bool equalTo(const Super &other) const {
            const VtIterator *it = dynamic_cast<const VtIterator *>(&other);
            return it && iter.equalTo(it->iter);
        }
        virtual bool lessThan(const Super &other) const {
            const VtIterator *it = dynamic_cast<const VtIterator *>(&other);
            return it && iter.lessThan(it->iter);
        }
        virtual void skip() { iter.skip();}

    protected:
        Iterator iter;
    };

    ///Iterator virtual wrapper
    /** Wraps any generic iterator into the virtual iterator.
     */
    template<class Iterator>
    class VtMutableIterator:
                public IVtMutableIterator<typename DeRefType<Iterator>::T::ItemT> {
    public:
        typedef IVtMutableIterator<typename DeRefType<Iterator>::T::ItemT> Super;
        typedef typename Super::ItemT ItemT;
        VtMutableIterator(Iterator iter):iter(iter) {}
        virtual bool hasItems() const {return iter.hasItems();}
        virtual natural getRemain() const {return iter.getRemain();}
        virtual const ItemT &getNext() {return iter.getNext();}
        virtual const ItemT &peek() const {return iter.peek();}
        virtual natural blockRead(ArrayRef<ItemT> &buffer) {
            return iter.blockRead(buffer);
        }
        virtual bool equalTo(const Super &other) const {
            const VtMutableIterator *it = dynamic_cast<const VtMutableIterator *>(&other);
            return it && iter.equalTo(it->iter);
        }
        virtual bool lessThan(const Super &other) const {
            const VtMutableIterator *it = dynamic_cast<const VtMutableIterator *>(&other);
            return it && iter.lessThan(it->iter);
        }
        virtual void skip() { iter.skip();}
        virtual ItemT& getNextMutable() {return iter.getNextMutable();}
        virtual ItemT& peekMutable() const {return iter.peekMutable();}

    protected:
        Iterator iter;
    };

    ///Iterator virtual wrapper
    /** Wraps any generic iterator into the virtual iterator.
     */
    template<class Iterator>
    class VtWriteIterator:
        public IVtWriteIterator<typename DeRefType<Iterator>::T::ItemT> {

    public:
        typedef IVtWriteIterator<typename DeRefType<Iterator>::T::ItemT> Super;
        typedef typename Super::ItemT ItemT;

        VtWriteIterator(Iterator iter):iter(iter) {}
        virtual void write(const ItemT &x) { iter.write(x);}
        virtual bool canAccept(const ItemT &x) const { return iter.canAccept(x);}
        virtual natural blockWrite(const ArrayRef<const ItemT> &buffer, bool writeAll = true) {
            return iter.blockWrite(buffer,writeAll);
        }

        virtual void flush() {iter.flush();}
        virtual void skip() {iter.skip();}

        virtual bool hasItems() const {return iter.hasItems();}
        virtual natural getRemain() const {return iter.getRemain();}
    protected:
        Iterator iter;
    };
    
}
#endif /*VTITERATOR_H_*/
