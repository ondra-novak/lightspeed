/*
 * queue.h
 *
 *  Created on: 23.7.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINER_QUEUE_H_
#define LIGHTSPEED_CONTAINER_QUEUE_H_


#include "deque.h"

namespace LightSpeed {

	template<class T,  natural citems = DequeClustSize<T>::n >
	class Queue
	{
	public:

		Queue() {}
		explicit Queue(NodeAlloc alloc):deque(alloc) {}

		void push(const T &x) {deque.pushBack(x);}
		const T &front() const {return deque.front();}
		void pop() {deque.popFront();}
		void clear() {deque.clear();}
		bool empty() const {return deque.empty();}

		natural length() const {return deque.length();}

		template<typename Archive>
		void serialize(Archive &arch) {
			arch(deque);
		}

		typedef typename Deque<T,citems>::Iterator Iterator;
		Iterator getFwIter() const {return deque.getFwIter();}

	protected:
		Deque<T,citems> deque;
	};


} // namespace LightSpeed

#endif /* QUEUE_H_ */
