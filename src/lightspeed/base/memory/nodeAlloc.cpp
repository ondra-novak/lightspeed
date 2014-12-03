/*
 * nodeAlloc.cpp
 *
 *  Created on: 8.2.2014
 *      Author: ondra
 */
#include "nodeAlloc.h"
#include "clusterAlloc.h"

namespace LightSpeed {


NodeAlloc createDefaultNodeAllocator() {
	return new ClusterAlloc;
}


}

