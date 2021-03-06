/*
 * iteratorFilter.h
 *
 *  Created on: 24.7.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ITERATORFILTER_H_
#define LIGHTSPEED_ITERATORFILTER_H_

#include "iterator.h"
#include "../exceptions/throws.h"
#include "iteratorChain.h"
#include "../qualifier.h"

namespace LightSpeed {


    ///Interface class to help you create filters for the filtering iterators
	/**
	 * FilterIterator is iterator which reads data from input source, makes some filtering or conversion and sends
	 * data to the output target. For each filter, there can be two iterators. Writing iterator ... which takes
	 * data passed to the function write() as input and results sends to the next writing iterator... or reading
	 * iterator ... which reads data from another reading iterator and result is returned by the function getNext().
	 *
	 * To allow write filters without need to implement both types of iterators, you can implement IIteratorFilter
	 * interface (by extending IteratorFilterBase). This is "the core" of the FilterWrite and FilterRead iterators
	 * so it is very simple to make both types of iterators from one filter
	 *
	 *
	 * @param InT type of the input objects
	 * @param OutT type of the output objects
	 * @param Derived name of final class that implements the filter
	 *
	 * @note to make new filter, extend IteratorFilterBase instead of IIteratorFilter
	 *
	 * @see IteratorFilterBase, FilterRead, FilterWrite
	 */
    template<typename InT, typename OutT, typename Derived>
    class IIteratorFilter: public Invokable<Derived> {
    public:

    	///Contains type of input object
        typedef InT InputT;
        ///Contains type of output object
        typedef OutT OutputT;

        typedef typename ConstObject<InT>::Remove MInT;
        typedef typename ConstObject<InT>::Add CInT;


        ///Tests, whether filter requests more items
        /**
         * @retval true filter need more items to generate results. This also mean, that filter can accept items,
         * 		for example, internal buffer is still not full.
         * @retval false filter will not accept items and will generate exception
         *
         * @note Please always use whole filter's memory, if it is possible. This mean, you should first
         * 		write items to the whole internal memory, then read all items generated by the filter until
         * 		memory is empty. This can give you better performance. Some filters also can return different
         * 		results, if they are not fully loaded before conversion starts
         */
        bool needItems() const {return this->_invoke().needItems();}

        ///Sends next object to the filter
        /**
         * @param x object to send into the filter
         *
         * @exception FilterBusyException Filter is busy, it has no more space to store the item.
         *
         * @note you should feed filter with the data until needItems() or
         *  canAccept() returns false. Immediatelly after last item has been fed,
         *  you should test hasItems and read result items. In 1:1 filters,
         *  for example TypeConversionFilter, there is no storage for the item,
         *  filters only stores pointer to the argument and expects, that
         *  output() will imediatelly follow input(). You should also
         *  keep variable x valid, until result item is not received. If
         *  needItems() returns true after first item is fed, filters is not 1:1.
         *  No storage optimization is allowed only for 1:1 filters, in other
         *  situations, usage of temporary storage is recomended.
         */
        void input(const InT &x) { this->_invoke().input(x);}

        ///Sends block of objects to the filter
        /**
         * @param block array of the object
         * @return count of object has been written. Function never returns zero. In case
         * that no more object can be stored function throws exception
         */
        template<typename Traits>
        natural blockInput(const FlatArray<MInT,Traits> &block) { return this->_invoke().blockInput(block);}
        template<typename Traits>
        natural blockInput(const FlatArray<CInT,Traits> &block) { return this->_invoke().blockInput(block);}
        template<typename Traits>
        natural blockFetch(IIterator<MInT,Traits> &iter) { return this->_invoke().blockFetch(iter);}
        template<typename Traits>
        natural blockFetch(IIterator<CInT,Traits> &iter) { return this->_invoke().blockFetch(iter);}
        ///Checks, whether filter has items for the output
        /**
         * @retval true there is at least one item for output
         * @retval false no items ready
         *
         * @note this function and function needItems() both can return false at the time. In this situation,
         * filter is closed and will not accept any objects more.
         */
        bool hasItems() const {return this->_invoke().hasItems();}
        ///Sends item to the output
        /**
         * Function returns object as result. This allows to use fast
         * object returning scheme, when filter will construct the object in
         * return statement and iterators will use result to construct
         * object in their local stack. Finally, there will be no copy constructor
         * and no assignment operators, it only depends on filter implementation.
         */
        OutT output() {return this->_invoke().output();}

        ///Retrieves block from the fukter
        /**
         * @param block array contains reserved count of objects. All objects must be constructed.
         *   Function is useful when binary objects are transfered, for example bytes. In this
         *   case, function is able to use block operations. Otherwise, result can be worse than
         *   reading object through output() function
         *
         * @return count of object has been written. Function never returns zero. In case
         * that no more object can be stored function throws exception
         */
        template<typename Traits>
        natural blockOutput(FlatArrayRef<OutT,Traits> block) { return this->_invoke().blockOutput(block);}
        template<typename Traits>
        natural blockFeed(IWriteIterator<OutT,Traits> &iter) { return this->_invoke().blockFeed(iter);}

        natural calcToWrite(natural srcCount) const
                                  {return this->_invoke().calcToWrite(srcCount);}
        natural calcToRead(natural trgCount) const
                                  {return this->_invoke().calcToRead(trgCount);}

        ///Flushes any filter's internal buffer
        /**
         * Filter is notified that flush operation has been requested. Filter should assume, that all
         * required objects has been received and prepare list of output objects. Iterators will soon read
         * object from the output list and deliver them to the output iterator to perform flush for next iterator
         * in the cascade
         * If any object is inserted after flush and before output list is whole consumed, filter should never combine
         * newly inserted object with objects prepared to output.
         */
        void flush() { this->_invoke().flush();}


    private:
        ///prevent automatic conversion on input
        template<typename X> void input(const X &x);

    };


    template<typename InT, typename OutT, typename Derived>
    class IteratorFilterBase: public IIteratorFilter<InT, OutT, Derived>
    {
    public:
    	typedef typename IIteratorFilter<InT, OutT, Derived>::MInT MInT;
    	typedef typename IIteratorFilter<InT, OutT, Derived>::CInT CInT;

        bool needItems() const;
        void input(const InT &x);
        bool hasItems() const;
        OutT output() ;
        natural calcToWrite(natural srcCount) const {return srcCount;}
        natural calcToRead(natural trgCount) const {return trgCount;}
        template<typename Traits>
        natural blockOutput(FlatArrayRef<OutT,Traits> block) {
        	return blockOutput(block.data(),block.length());
        }
        template<typename Traits>
        natural blockInput(const FlatArray<typename ConstObject<InT>::Remove,Traits> &block) {
        	return blockInput(block.data(), block.length());
        }
        template<typename Traits>
        natural blockInput(const FlatArray<typename ConstObject<InT>::Add,Traits> &block) {
        	return blockInput(block.data(), block.length());
        }

        template<typename Traits>
        natural blockFetch(IIterator<MInT,Traits> &iter) {
        	natural cnt = 0;
        	while (this->_invoke().needItems() && iter.hasItems()) {
        		this->_invoke().input(iter.getNext());
        		cnt++;
        	}
        	if (this->_invoke().needItems()) this->_invoke().flush();
        	return cnt;
        }
        template<typename Traits>
        natural blockFetch(IIterator<CInT,Traits> &iter) {
        	natural cnt = 0;
        	while (this->_invoke().needItems() && iter.hasItems()) {
        		this->_invoke().input(iter.getNext());
        		cnt++;
        	}
        	if (this->_invoke().needItems()) this->_invoke().flush();
        	return cnt;
        }

        template<typename Traits>
        natural blockFeed(IWriteIterator<OutT,Traits> &iter) {
        	natural cnt = 0;
        	while (this->_invoke().hasItems() && (cnt == 0 || iter.hasItems())) {
        		iter.write(this->_invoke().output());
        		cnt++;
        	}
        	return cnt;
        }

        void flush() {}
    private:
        ///prevent automatic conversion on input
        template<typename X> void input(const X &x);


        natural blockInput(const InT *start, natural len) {
        	natural c = 0;
        	do {
        		this->_invoke().input(start[c++]);
        	} while (c<len && this->_invoke().needItems());
        	return c;
        }


        natural blockOutput(OutT *start, natural len) {
        	natural c = 0;
        	do {
        		start[c++] = this->_invoke().output();
        	} while (c<len && this->_invoke().hasItems());
        	return c;
        }
};


    ///Helps to create filter chains
    /** Using this class, you can create filter chain. Each
     * item in the chain is packed into the final object.
     * Constructor then takes only instance of the first
     * iterator and instance itself can be
     * used as source of another chain or can be used
     * to manipulate with the data
     *
     * @param InputFlt Input iterator type. It can
     *  be any possible iterator, or any FltChain
     *  or FltChain &. If you use FltChain &, you requests
     *  to create reference chain (input is accessed
     *  using reference. In this case, filter part is instantiated
     *  as member variable of chain item and next
     *  instance is initialized as reference.
     *
     *  @param OutputFlt Result (output) iterator of
     *  the filter. Note that it doesn't mean direcion
     *  of processing. Output iterator is name class that
     *  will be extended by FltChain and which interface program will
     *  finally access. FltChain<StdInput,ReadBuffer> will
     *  receive interface to ReadBuffer which access StdInput. Also
     *  FltChain<StdOutput, WriteBuffer> will receive interface to
     *  WriteBuffer which access StdOutput
     */
    template<typename InputFlt, typename OutputFlt>
    class FltChain: public OutputFlt {
    public:
        FltChain(InputFlt iter):OutputFlt(iter) {}
        typedef InputFlt FltChainedType ;

        ///Allows to iterate and access through the chain items
        /**
         * @return reference to the iterator, which this chain-item controls.
         * FltChain allows chaining this function. If you want to access
         * third iterator in the chain, call iter.chain().chain() )(twice,
         * because first is iter). Last item of chain is very first input iterator
         */
        FltChainedType &nxChain() {return OutputFlt::nxChain();}
        ///Allows to iterate and access through the chain items
        /**
         * @return const reference to the iterator, which this chain-item controls.
         * FltChain allows chaining this function. If you want to access
         * third iterator in the chain, call iter.chain().chain() )(twice,
         * because first is iter). Last item of chain is very first input iterator
         */
        const FltChainedType &nxChain() const {return OutputFlt::nxChain();}

    };


    template<typename A, typename B, typename OutputFlt>
    class FltChain<FltChain<A,B> &, OutputFlt>: public OutputFlt {
        FltChain<A,B> wrk;
    public:
        typedef typename FltChain<A,B>::FltChainedType FltChainedType ;
        FltChain(FltChainedType iter) :OutputFlt(wrk),wrk(iter) {}
        FltChain(): OutputFlt(wrk) {}

        FltChain<A,B> &getChainedIterator() {
            return wrk;
        }
        const FltChain<A,B> &getChainedIterator() const {
            return wrk;
        }

    };

    template<typename A, typename B, typename OutputFlt>
    class FltChain<FltChain<A,B>, OutputFlt >: public OutputFlt {
    public:
        typedef typename FltChain<A,B>::FltChainedType FltChainedType ;
        FltChain(FltChainedType  iter)
            :OutputFlt(FltChain<A,B>(iter)) {}
        FltChain():OutputFlt(FltChain<A,B>()) {}

        FltChain<A,B> &nxChain() {
            return static_cast<FltChain<A,B> &>(OutputFlt::nxChain());
        }
        const FltChain<A,B> &nxChain() const {
            return static_cast<const FltChain<A,B> &>(OutputFlt::nxChain());
        }
    };


	///Null filter doesn't perform any conversion
	/** It needs to support TypeConversionFilter works standalone */
	template<typename T>
	class NullFilter: public IteratorFilterBase<T,T,NullFilter<T> >{
	public:
		bool needItems() const {return item == 0;}
		bool canAccept(const T &) const {return true;}
		void input(const T &x) {item = &x;}
		bool hasItems() const {return item != 0;}
		T output() {
			const T *x = item;
			item = 0;
			return *x;
		}
		NullFilter():item(0) {}
	protected:
		const T *item;
    private:
        ///prevent automatic conversion on input
        template<typename X> void input(const X &x);
	};

    ///Provides type conversion in iteration
    /**
     * Receives items of the one time and returns items of the other type. Expects
     * that there is defined conversion using proper constructor or
     * operator.
     *
     * @param InT input type
     * @param OutT output type
     *
     * Example TypeConversionFilter<byte,char> .... converts bytes to characters
     *
     * @note 1:1 filter with "no internal storage" optimization. After calling
     * input() you should call output() with keeping variable of input() valid.
     * Template classes FilterRead and FilterWrite can handle this rule.
     *
     */
    template<typename InT, typename OutT, typename BaseFilter = NullFilter<InT> >
    class TypeConversionFilter:
            public IteratorFilterBase<InT, OutT, TypeConversionFilter<InT,OutT,BaseFilter> >
    {
	public:
        bool needItems() const {return flt.needItems();}
        void input(const InT &x) {flt.input(x);} //NOTE Don't put type conversion here!!!
        bool hasItems() const {return flt.hasItems();}
		OutT output() {return OutT(flt.output());}        

        TypeConversionFilter() {}
		TypeConversionFilter(const BaseFilter &flt):flt(flt) {}
    protected:
        BaseFilter flt;
    private:
        ///prevent automatic conversion on input
        template<typename X> void input(const X &x);

    };




    ///This is proxy iterator, that supports filters.
    /** In combination with this class and filter class, you can create filtering iterator, which
     * is able to read items in different type than it is returned
     */
    template<typename RdIterator, typename Filter>
    class FilterRead: public IteratorChain<RdIterator,FilterRead<RdIterator,Filter>,
                             IteratorBase, typename DeRefType<Filter>::T::OutputT>
    {
    public:
        typedef IteratorChain<RdIterator,FilterRead<RdIterator,Filter>,
			::LightSpeed::IteratorBase, typename DeRefType<Filter>::T::OutputT> Super;

        typedef typename DeRefType<Filter>::T::OutputT OutputT;

    	///Constructs filter reader
    	/**
    	 * @param rd reading iterator contains source data
    	 * @param askAccept if true, iterator can use peek() to pre-fetch data and test it with canAccept(). Use
    	 * 			false, if you know, that calling canAccept() is not necessery, or if input iterator doesn't
    	 * 			support peek()
    	 */
        FilterRead(RdIterator rd);

    	///Constructs filter readertemplate class FilterTextParser<natural>;

    	/**
    	 * @param rd reading iterator contains source data
    	 * @param flt filter instance
    	 * @param askAccept if true, iterator can use peek() to pre-fetch data and test it with canAccept(). Use
    	 * 			false, if you know, that calling canAccept() is not necessery, or if input iterator doesn't
    	 * 			support peek()
    	 */
        FilterRead(RdIterator rd, Filter flt);

    	///Destructor
        ~FilterRead();


        bool hasItems() const;
        natural getRemain() const {return flt.calcToRead(this->iter.getRemain());}
        const OutputT &peek() const;
        const OutputT &getNext() ;
        template<class Traits>
        natural blockRead(FlatArrayRef<OutputT,Traits> buffer, bool readAll);


        Filter &getFilterInstance() {return flt;}
        const Filter &getFilterInstance() const {return flt;}


    protected:
        mutable Filter flt;
        //buffer contains space for the item ...
        //using directly variable is not clever, because it needs default constructor.
        mutable char buffer[sizeof(OutputT)];
        //fetched: =0 ~ empty, =1 ~ fetched and used, =2 ~ fetched and unused (can be deleted)
        mutable char fetched;

        const OutputT &getBuffer() const;
        void freeBuffer() const;
        void prefreeBuffer();

    };


    ///This is proxy write iterator, that supports filters.
    /** In combination with this class and filter class, you can create filtering write iterator, which
     * is able to write items in different type than it is written using function write()
     */
    template<typename WrIterator, typename Filter>
    class FilterWrite: public IteratorChain<WrIterator,FilterWrite<WrIterator,Filter>,
                            WriteIteratorBase, typename DeRefType<Filter>::T::InputT>
    {
    public:

        typedef IteratorChain<WrIterator,FilterWrite<WrIterator,Filter>,
			::LightSpeed::WriteIteratorBase, typename DeRefType<Filter>::T::InputT> Super;


        typedef typename DeRefType<Filter>::T::InputT InputT;
    	///Constructs filter reader
    	/**
    	 * @param wr writing iterator contains target stream
    	 */
        FilterWrite(WrIterator wr);

    	///Constructs filter reader
    	/**
    	 * @param wr writing iterator contains target stream
    	 * @param flt filter instance
    	 */
        FilterWrite(WrIterator wr, Filter flt);

        ///Destructor
        /** Additionally flushes any unclosed buffer */
        ~FilterWrite();


        bool hasItems() const;
        natural getRemain() const {return flt.calcToWrite(this->iter.getRemain());}
        void write(const InputT &inp);
        void flush();

        template<class Traits>
        natural blockWrite(const FlatArray<typename ConstObject<InputT>::Remove,Traits> &buffer, bool writeAll = true);
        template<class Traits>
        natural blockWrite(const FlatArray<typename ConstObject<InputT>::Add,Traits> &buffer, bool writeAll = true);


    protected:
        mutable Filter flt;

        void flushCycle();
    };




    ///Template generates both reader and writer for given filter
    /**
     * It allows you to use result iterator directly in the filter chain.
     *
     * Example:  typedef Filter<MyFilter> MyFilterIters;
     *           MyFilterIters::Read<Iterator> iter(chainedIter);
     *
     */
    template<typename Flt>
    class Filter {
    public:
        ///Reading iterator (throught the filter)
        template<typename Iterator>
        class Read: public FilterRead<Iterator,Flt> {
        public:
            Read(Iterator iter):FilterRead<Iterator,Flt>(iter) {}
            Read(Iterator iter, const Flt &flt):FilterRead<Iterator,Flt>(iter,flt) {}

        };
        ///Writing iterator (throught the filter)
        template<typename Iterator>
        class Write: public FilterWrite<Iterator,Flt> {
        public:
            Write(Iterator iter):FilterWrite<Iterator,Flt>(iter) {}
            Write(Iterator iter, const Flt &flt):FilterWrite<Iterator,Flt>(iter,flt) {}
        };
    };


    template<typename Iter, typename From, typename To>
    class ConvFilter {
    public:
    	typedef typename Filter<TypeConversionFilter<From,To> >::template Read<Iter> Reader;
    	typedef typename Filter<TypeConversionFilter<From,To> >::template Write<Iter> Writer;
    };

    // -------------------- implementation --------------

}

#endif /* ITERATORFILTER_H_ */















