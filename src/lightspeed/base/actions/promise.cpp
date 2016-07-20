#include "promise.h"
#include "promise.tcc"

#include "../memory/poolalloc.h"

namespace LightSpeed {

	template class Future<void>;
	template class Promise<void>;

Future<void> Future<void>::then(const Promise<void> &resolution) {
	return Future<Void>::then(static_cast<const Promise<Void> &>(resolution));
}
	


Promise<void> Future<void>::getPromise()
{
	return Future<Void>::getPromise();
}

IRuntimeAlloc * getPromiseAlocator()
{
	// use singleton to allocate promise's pool
	return &Singleton<PoolAlloc>::getInstance();
}

}
