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



static RuntimeAllocKeeper &getKeeper() {
	static byte space[sizeof(PoolAlloc)];
	static RuntimeAllocKeeper keeper(new(&space) PoolAlloc);
	return keeper;
}



IRuntimeAlloc& IPromiseControl::getAllocator() {
	return getKeeper().getInstance();

}

void IPromiseControl::setAllocator(IRuntimeAlloc* alloc) {
	getKeeper().setInstance(alloc);
}

IRuntimeAlloc &getWeakRefAllocator() {
	return getKeeper().getInstance();
}


}
