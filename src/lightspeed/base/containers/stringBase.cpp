#include "stringBase.h"
#include "../memory/stdAlloc.h"

namespace LightSpeed {

	///Pointer to allocate allocator responsible to allocate strings
	/** pointer is set to StdAlloc on first usage */
	static IRuntimeAlloc *stringDefAlloc = 0;


	IRuntimeAlloc &getStringDefaultAllocator() {
		if (stringDefAlloc == 0) stringDefAlloc = &StdAlloc::getInstance();
		return *stringDefAlloc;
	}

	void setStringDefaultAllocator( IRuntimeAlloc &alloc )
	{
		stringDefAlloc = &alloc;
	}



}
