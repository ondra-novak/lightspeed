/*
 * sortFilter.h
 *
 *  Created on: 24.8.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_SORTFILTER_H_
#define _LIGHTSPEED_SORTFILTER_H_

#include "../exceptions/outofmemory.h"
#include "iteratorFilter.h"
#include "../containers/sort.h"
#include "../containers/autoArray.h"
#include "../containers/optional.h"
#include <functional>

namespace LightSpeed {


    ///Filter that allows to create sorted streams
    /** If used  with thr Filter class, you can create filter that
     * accepts unsorted data and returns sorted. You can
     * also use this filter standalone and you receives object
     * that works similar to priority queue
     *
     * Note that, if this filter is used with FilteRead, you
     * cannot read infinite stream, because filter first collects
     * all input data and then it is able to return sorted version. You
     * have to insert some "limiter" that will limit count of data, or
     * use StAllocator with limited buffer. Filter reports
     * needItems until internal buffer accepts the items. It is
     * by default AutoArray with unlimited allocator.
     *
     * If filter is used with the FilterWrite, all data written into
     * this filter are not send to the output, until flush() command is invoked.
     * Flushing is also performed during destruction. When flush is
     * invoked, filter outputs all data in the sorted order
     */
    template<typename T, typename Order = std::less<T>, typename Allocator = StdAlloc >
    class SortFilter: public IteratorFilterBase< T, T, SortFilter<T,Order,Allocator> >
    {
    public:
        ///Constructs sort filter
        SortFilter();
        ///Constructs sort filter and initialize comparator
        /**
         * @param order an instance of comparator
         */
        SortFilter(const Order &order, Allocator alloc = Allocator());

        SortFilter(const SortFilter &other);

        bool needItems() const;
        bool canAccept(const T &) const {return true;}
        void input(const T &x);
        bool hasItems() const;
        T output() ;
		void clear() {buffer.clear();heap.reset();}
		natural length() const {return buffer.length();}
		bool empty() const {return buffer.empty();}

        SortFilter &operator=(const SortFilter &other);

    protected:
        typedef HeapSort<AutoArray<T,Allocator>, Order> HeapT;
        AutoArray<T,Allocator> buffer;
        Optional<T> extraItem;
        HeapT heap;
    };


}

#endif /* _LIGHTSPEED_SORTFILTER_H_ */
