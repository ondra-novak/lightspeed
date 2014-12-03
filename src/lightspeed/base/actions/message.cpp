/*
 * message.cpp
 *
 *  Created on: 7. 10. 2014
 *      Author: ondra
 */


#include "message.h"

#include "../memory/poolalloc.h"
#include "../memory/singleton.h"

namespace LightSpeed {
///Pointer to allocate allocator responsible to allocate strings
/** pointer is set to StdAlloc on first usage */
static IRuntimeAlloc *messageDefAlloc = 0;


IRuntimeAlloc &getMessageDefaultAllocator() {
	if (messageDefAlloc == 0) messageDefAlloc = &Singleton<MultiPoolAlloc>::getInstance();
	return *messageDefAlloc;
}

void setMessageDefaultAllocator( IRuntimeAlloc &alloc )
{
	messageDefAlloc = &alloc;
}



}

