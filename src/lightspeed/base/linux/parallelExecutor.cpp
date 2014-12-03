/*
 * parallelExecutor.cpp
 *
 *  Created on: 10.4.2011
 *      Author: ondra
 */

#include "../actions/parallelExecutor.h"
#include <unistd.h>
#include <sys/sysinfo.h>

namespace LightSpeed {

natural ParallelExecutor::getCPUCount() {
	int res = sysconf(_SC_NPROCESSORS_ONLN);
//	int res = get_nprocs();
	if (res == -1) return 0;
	else return res;
}


}
