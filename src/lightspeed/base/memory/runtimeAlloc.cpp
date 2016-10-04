/*
 * runtimeAlloc.cpp
 *
 *  Created on: 28. 9. 2016
 *      Author: ondra
 */

#include "runtimeAlloc.h"

#include "../../mt/atomic.h"
namespace LightSpeed {

void RuntimeAllocKeeper::setInstance(IRuntimeAlloc *a) {
	IRuntimeAlloc * volatile old = lockExchangePtr(&alloc,a);
	if (old != a) {
		old->release();
	}
}


}


