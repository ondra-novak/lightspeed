#include "promise.h"
#include "promise.tcc"

#include "../memory/poolalloc.h"

namespace LightSpeed {

template class Promise<void>;

Promise<void> Promise<void>::then(const PromiseResolution<void> &resolution) {
	return Promise<Empty>::then(static_cast<const PromiseResolution<Empty> &>(resolution));
}
	


IRuntimeAlloc * getPromiseAlocator()
{
	static PoolAlloc pool;
	return &pool;
}

}
