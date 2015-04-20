#include "promise.h"
#include "promise.tcc"

#include "../memory/poolalloc.h"

namespace LightSpeed {

template class Promise<int>;	

	


IRuntimeAlloc * getPromiseAlocator()
{
	static PoolAlloc pool;
	return &pool;
}

}
