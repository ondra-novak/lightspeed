/*
 * dispatcher.cpp
 *
 *  Created on: 30. 7. 2015
 *      Author: ondra
 */




#include "idispatcher.tcc"
#include "message.h"

namespace LightSpeed {
	
	typedef Message<int> TestFn;

	template void IDispatcher::dispatch(const TestFn &);
	template void IDispatcher::dispatchPromise(const Promise<int> &, const Promise<int>::Result &);
	template void IDispatcher::dispatch<TestFn, int>(const TestFn &, const Promise<int>::Result &);

}
