/*
 * priority_queue.h
 *
 *  Created on: May 23, 2016
 *      Author: ondra
 */

#ifndef LIBS_LIGHTSPEED_SRC_LIGHTSPEED_BASE_CONTAINERS_PRIORITYQUEUE_H_
#define LIBS_LIGHTSPEED_SRC_LIGHTSPEED_BASE_CONTAINERS_PRIORITYQUEUE_H_
#include "../containers/sort.h"

#include "../containers/autoArray.h"

namespace LightSpeed {

	template<class T, class Cmp = std::less<T>, class Allocator = StdAlloc>
	class PriorityQueue
	{
	public:

		PriorityQueue():heap(items) {}
		explicit PriorityQueue(const Cmp &cmp):heap(items,cmp) {}
		explicit PriorityQueue(const Allocator &alloc):items(alloc),heap(items) {}
		PriorityQueue(const Allocator &alloc,const Cmp &cmp):items(alloc),heap(items,cmp) {}

		void push(const T &x) {items.add(x);heap.push();}

		template<typename I>
		void push(const Constructor<T, I> &c) {items.add(c);heap.push();}

		const T &top() const { return items[0]; }
		T &top() { return items[0]; }
		void pop() { heap.pop(); items.trunc(heap.getSize()); }
		void clear() {heap.clear();items.clear();}
		bool empty() const {return items.empty();}

		natural length() const {return heap.getSize();}


	protected:
		AutoArray<T,Allocator> items;
		HeapSort<AutoArray<T>, Cmp> heap;
	};


} // namespace LightSpeed




#endif /* LIBS_LIGHTSPEED_SRC_LIGHTSPEED_BASE_CONTAINERS_PRIORITYQUEUE_H_ */
