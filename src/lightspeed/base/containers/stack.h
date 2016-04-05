/*
 * stack.h
 *
 *  Created on: 23.7.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_CONTAINER_STACK_H_
#define LIGHTSPEED_CONTAINER_STACK_H_


#include "deque.h"

namespace LightSpeed {

	
    template<class T, class Allocator = StdAlloc >
    class Stack
    {
    public:

    	Stack() {}

		void push(const T &x) {deque.pushBack(x);}

		template<typename I>
		void push(const Constructor<T, I> &c) { deque.pushBack(c); }

		const T &top() const {return deque.getBack();}
		void pop() {deque.popBack();}
		void clear() {deque.clear();}
		bool empty() const {return deque.empty();}

		natural length() const {return deque.length();}

    protected:
        Deque<T,Allocator > deque;
    };

} // namespace LightSpeed

#endif /* STACK_H_ */
