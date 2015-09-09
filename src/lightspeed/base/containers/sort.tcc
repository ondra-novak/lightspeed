/*
 * sort.tcc
 *
 *  Created on: 27.8.2010
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_SORT_TCC_
#define _LIGHTSPEED_SORT_TCC_

#include "sort.h"

namespace LightSpeed {

///Sorts array using heap sort
template<class Container,class Cmp>
void HeapSort<Container,Cmp>::sort() {
    makeHeap();
    sortHeap();
}

template<class Container,class Cmp>
void HeapSort<Container,Cmp>::makeHeap() {
    while (heapSize < cont.length())
        push();
}

template<class Container,class Cmp>
void HeapSort<Container,Cmp>::sortHeap() {
    while (heapSize > 0)
        pop();
}

template<class Container,class Cmp>
bool HeapSort<Container,Cmp>::push() {
    if (heapSize < cont.length()) {
        natural child = heapSize  ;
        while (child > 0) {
            natural parent = (child - 1) / 2;
            if (isLess(cont[parent],cont[child])) {
                std::swap(cont(parent),cont(child));
                child = parent;
            } else
                break;
        }
        heapSize++;
        return true;
    } else {
        return false;
    }
}

template<class Container,class Cmp>
bool HeapSort<Container,Cmp>::pop() {
    if (heapSize > 0) {
        heapSize--;
        if (heapSize > 0) {
            std::swap(cont(0),cont(heapSize));
            shiftDown();
        }
        return true;
    } else {
        return false;
    }
}

template<class Container,class Cmp>
bool HeapSort<Container,Cmp>::pop(natural at) {
    if (heapSize > at) {
        heapSize--;
        if (heapSize > at) {
            std::swap(cont(at),cont(heapSize));
            shiftDown();
        }
        return true;
    } else {
        return false;
    }
}


template<class Container,class Cmp>
void HeapSort<Container,Cmp>::shiftDown() {
	shiftDown(0);
}

template<class Container,class Cmp>
void HeapSort<Container,Cmp>::shiftDown(natural from) {
    if (heapSize > from*2+1) {
        natural root = from;
        natural child;
        while ((child = root * 2 + 1) < heapSize) {
            if (child + 1 < heapSize && isLess(cont[child],cont[child+1]))
                child++;
            if (isLess(cont[root],cont[child])) {
                std::swap(cont(root),cont(child));
                root = child;
            } else
                break;
        }
    }
}

///Retrieves top of head
template<class Container,class Cmp>
const typename HeapSort<Container,Cmp>::ItemT &HeapSort<Container,Cmp>::top() const {
    return cont[0];
}

template<class Container,class Cmp>
natural HeapSort<Container,Cmp>::getSize() const {
    return heapSize;
}

template<class Container,class Cmp>
void HeapSort<Container,Cmp>::setSize(natural sz){
    heapSize = sz;
}

template<class Container,class Cmp>
bool HeapSort<Container,Cmp>::isBetter(const ItemT &x) const {
    return isLess(top(),x);
}


}
#endif /* SORT_TCC_ */
