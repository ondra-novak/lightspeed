/*
 * parallelExecutor.cpp
 *
 *  Created on: 4.4.2011
 *      Author: ondra
 */

#include "parallelExecutor.h"


namespace LightSpeed {


ParallelExecutor::ParallelExecutor(natural maxThreads,natural maxWaitTimeout)
:ParallelExecutor2(maxThreads,maxWaitTimeout,0,60000)
{
}


}
