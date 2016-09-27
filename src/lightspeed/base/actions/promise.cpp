#include "promise.h"
#include "promise.tcc"

#include "../memory/poolalloc.h"

namespace LightSpeed {

	template class Future<void>;
	template class Promise<void>;

Future<void> Future<void>::then(const Promise<void> &resolution) {
	return Future<Void>::then(static_cast<const Promise<Void> &>(resolution));
}
	

Future<void>::Future(IRuntimeAlloc &alloc):Future<Void>(alloc) {}



Promise<void> Future<void>::getPromise()
{
	return Future<Void>::getPromise();
}


static PoolAlloc poolAlloc;
static IRuntimeAlloc *curAlloc = &poolAlloc;

IRuntimeAlloc& IPromiseControl::getAllocator() {
	return *curAlloc;

}

void IPromiseControl::setAllocator(IRuntimeAlloc* alloc) {
	if (alloc == 0) curAlloc = &poolAlloc;
	else curAlloc = alloc;
	poolAlloc.freeExtra();

}

}
