/*
 * nodeAlloc.h
 *
 *  Created on: 8.2.2014
 *      Author: ondra
 */

#ifndef LIGHTSPEED_NODEALLOC_H_
#define LIGHTSPEED_NODEALLOC_H_
#include "../memory/sharedPtr.h"

namespace LightSpeed {


typedef SharedPtr<IRuntimeAlloc> NodeAlloc;
NodeAlloc createDefaultNodeAllocator();



}



#endif /* LIGHTSPEED_NODEALLOC_H_ */
