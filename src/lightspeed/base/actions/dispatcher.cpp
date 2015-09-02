/*
 * dispatcher.cpp
 *
 *  Created on: 30. 7. 2015
 *      Author: ondra
 */




#include "dispatcher.tcc"

namespace LightSpeed {

template Promise<typename AbstractDispatcher::DispatchHelper<Promise<int> >::RetV> AbstractDispatcher::dispatch(const Promise<int> &);



void AbstractDispatcher::registerPromise(PromiseReg* reg) {
	Synchronized<FastLock> _(unregLock);

	reg->next = regChain;
	reg->prev = 0;
	regChain = reg;
}

void AbstractDispatcher::unregisterPromise(PromiseReg* reg) {
	Synchronized<FastLock> _(unregLock);
	if (reg == regChain) {
		regChain = reg->next;
	}
	if (reg->prev) reg->prev->next = reg->next;
	if (reg->next) reg->next->prev = reg->prev;
	reg->next = reg->prev = 0;
}

void AbstractDispatcher::cleanup() {
	Synchronized<FastLock> _(unregLock);
	while (regChain) {
		PromiseReg *r = regChain;
		regChain = r;


	}
}

}

