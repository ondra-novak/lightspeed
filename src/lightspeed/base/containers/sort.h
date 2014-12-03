/*
 * sort.h
 *
 *  Created on: 21.8.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_SORT_H_
#define _LIGHTSPEED_SORT_H_
#include <functional>


namespace LightSpeed {

    template<class Container,
        class Cmp = std::less<typename Container::ItemT> >
    class HeapSort {
    public:

        typedef typename Container::ItemT ItemT;

        HeapSort(Container &cont):cont(cont),heapSize(0) {}
        HeapSort(Container &cont, const Cmp &isLess):cont(cont),isLess(isLess),heapSize(0) {}

        ///Sorts array using heap sort
        void sort();

        ///Creates heap on the array
        /**
         * Creates heap on the array. First item is maximum followed by
         * typical heap structure. You have to call this function
         * before calling push or pop function
         */
        void makeHeap();

        ///Sorts heap
        void sortHeap();


        ///pushes item into heap
        /** it takes next item from the container, and moves it
         * to the right position in the heap
         * @retval true okay
         * @retval false no more items in the container
         */
        bool push();

        ///retrieves current heap size
        /**
         * @return size of heap
         */
        natural getSize() const;

        void setSize(natural size);

        bool empty() const {return getSize() == 0;}

        ///Retrieves top of head
        const ItemT &top() const;


        bool pop();


        ///Rebuilds heap due changing value of the top item
        /**
         * Function is faster than full reheap calling makeHeap(), because
         * it only reorders one branch of heap. After calling, new top is
         * placed as first item
         */
        void shiftDown();

        ///Tests, whether specified item is better than heap
        /**
         * @param item item to test
         * @retval true item is better than top (adding it into heap causes
         *   that object will be new top
         * @retval false item is not better than top
         *
         * Function is designed to allow fast test, whether object is
         * better without adding it to the heap followed by remove.
         */
        bool isBetter(const ItemT &item) const;

        Cmp getCmpOp() const {return isLess;}

		///resets object state - treat container not sorted
		void reset() {heapSize = 0;}
    protected:
        Container &cont;
        Cmp isLess;
        natural heapSize;
    };


    ///Reverses compare operator
    /** Ordering will be reversed against to operator */
    template<typename Cmp>
    class ReversedOrder {

    	ReversedOrder(const Cmp &cmp):cmp(cmp) {}
    	template<typename T>
    	bool operator()(const T &a, const T &b) const{
    		return cmp(b,a);
    	}

    protected:
    	Cmp cmp;
    };
}


#endif /* _LIGHTSPEED_SORT_H_ */
