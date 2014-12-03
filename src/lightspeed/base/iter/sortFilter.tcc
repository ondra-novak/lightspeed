/*
 * sortFilter.tcc
 *
 *  Created on: 6.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ITER_SORTFILTER_TCC_
#define LIGHTSPEED_ITER_SORTFILTER_TCC_

#include "sortFilter.h"
#include "../containers/sort.tcc"
#include "../containers/autoArray.tcc"
#include "../exceptions/outofmemory.h"
#include "../exceptions/iterator.h"

namespace LightSpeed {


template<typename T, typename Order, typename Allocator>
SortFilter<T,Order,Allocator>::SortFilter()
    :heap(buffer) {

}
template<typename T, typename Order, typename Allocator>
SortFilter<T,Order,Allocator>::SortFilter(const Order &order,Allocator alloc)
    :buffer(alloc),heap(buffer,order)  {

}

template<typename T, typename Order, typename Allocator>
inline bool SortFilter<T,Order,Allocator>::needItems() const
{
    return !extraItem.isSet();
}



template<typename T, typename Order, typename Allocator>
inline void SortFilter<T,Order,Allocator>::input(const T & x)
{
    if (extraItem.isSet()) {
        try {
            buffer.add(extraItem.get());
            heap.push();
            extraItem.unset();
        } catch (AllocatorLimitException &e) {
            throw WriteIteratorNoSpace(THISLOCATION,typeid(T)) << e;
        }
    }

    try {
        buffer.add(x);
        heap.push();
    } catch (AllocatorLimitException &) {
        extraItem.set(x);
    }
}



template<typename T, typename Order, typename Allocator>
inline bool SortFilter<T,Order,Allocator>::hasItems() const
{
    return !buffer.empty();
}


template<typename T, typename Order, typename Allocator>
SortFilter<T,Order,Allocator>::SortFilter(const SortFilter &other)
    :buffer(other.buffer),heap(buffer, other.heap.getCmpOp()) {

}

template<typename T, typename Order, typename Allocator>
SortFilter<T,Order,Allocator> &SortFilter<T,Order,Allocator>::operator=(const SortFilter &other) {
    buffer = other.buffer;
    heap.setSize(buffer.length());
    return *this;
}

template<typename T, typename Order, typename Allocator>
inline T SortFilter<T,Order,Allocator>::output()
{
    class AutoPop {
    public:
        HeapT &heap;
		AutoArray<T,Allocator> &buffer;
        AutoPop(HeapT &heap,AutoArray<T,Allocator> &buffer):heap(heap),buffer(buffer) {}
        ~AutoPop() {heap.pop();buffer.trunc(1);}
    };

    class AutoUnset {
    public:
        Optional<T> &extraItem;
        AutoUnset(Optional<T> &extraItem):extraItem(extraItem) {}
        ~AutoUnset() {extraItem.unset();}
    };

    if (extraItem.isSet()) {
        if (heap.isBetter(extraItem.get())) {
            AutoUnset unset(extraItem);
            return extraItem.get();
        } else {
            std::swap(extraItem.get(),buffer(0));
            heap.shiftDown();
            AutoUnset unset(extraItem);
            return extraItem.get();
        }
    } else {
        if (buffer.empty())
            throw IteratorNoMoreItems(THISLOCATION,typeid(T));
        AutoPop pop(heap,buffer);
        return heap.top();
    }

}



}  // namespace LightSpeed

#endif /* LIGHTSPEED_ITER_SORTFILTER_TCC_ */
