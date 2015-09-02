#include "promise.h"
#include "promise.tcc"

#include "../memory/poolalloc.h"

namespace LightSpeed {

template class Promise<void>;

	


IRuntimeAlloc * getPromiseAlocator()
{
	static PoolAlloc pool;
	return &pool;
}

}
