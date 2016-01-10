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

	template<class T>
	class Queue
	{
	public:

		Queue() {}

		void push(const T &x) {deque.pushBack(x);}

		template<typename I>
		void push(const Constructor<T, I> &c) { deque.pushBack(c); }

		const T &top() const { return deque.getFront(); }
		T &top() { return deque.getFront(); }
		void pop() { deque.popFront(); }
		void clear() {deque.clear();}
		bool empty() const {return deque.empty();}

		natural length() const {return deque.length();}


	protected:
		Deque<T> deque;
	};


} // namespace LightSpeed

#endif /* QUEUE_H_ */
