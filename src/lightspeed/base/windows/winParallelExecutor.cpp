/*
 * parallelExecutor.cpp
 *
 *  Created on: 10.4.2011
 *      Author: ondra
 */

#include "winpch.h"
#include "../actions/parallelExecutor.h"


namespace LightSpeed {

natural ParallelExecutor::getCPUCount() {
	SYSTEM_INFO sinfo;
	initStruct(sinfo);
	GetSystemInfo(&sinfo);
	return sinfo.dwNumberOfProcessors;
}


}
