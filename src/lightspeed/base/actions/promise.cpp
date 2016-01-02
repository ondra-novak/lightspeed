#include "promise.h"
#include "promise.tcc"

#include "../memory/poolalloc.h"

namespace LightSpeed {

	template class Future<void>;
	template class Promise<void>;

Future<void> Future<void>::then(const Promise<void> &resolution) {
	return Future<Empty>::then(static_cast<const Promise<Empty> &>(resolution));
}
	


Promise<void> Future<void>::getPromise()
{
	return Future<Empty>::getPromise();
}

IRuntimeAlloc * getPromiseAlocator()
{
	static PoolAlloc pool;
	return &pool;
}

}
