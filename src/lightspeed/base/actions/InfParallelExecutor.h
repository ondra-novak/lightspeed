/*
 * InfParallelExecutor.h
 *
 *  Created on: Nov 6, 2012
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ACTIONS_INFPARALLELEXECUTOR_H_
#define LIGHTSPEED_ACTIONS_INFPARALLELEXECUTOR_H_
#include "../../mt/thread.h"
#include "../memory/sharedPtr.h"
#include "executor.h"
#include "../containers/autoArray.h"
#include "../memory/clusterAlloc.h"

namespace LightSpeed {


///Infinite parallel executor
/**
 * Simplified parallel executor useful to create unlimited count
 * of threads. In compare to Parallel executor, there is no finite count
 * of running threads. Every action is executed in separate thread which is
 * created on deamand. After action is finished, thread is terminated - this
 * is difference from Parallel executor which keeps thread running and waiting
 * for next task.
 *
 * InfParallelExecutor can be slower than ParallelExecutor and create a lot
 * of threads. It is useful when you need to allocate extra thread for
 * single action.
 *
 * Executor tracks all running threads. In destructor, all threads
 * are signaled to terminate. Executor waits until last thread finish its work.
 */
class InfParallelExecutor: public IExecutor {
public:

	virtual void execute(const IExecAction &action);;
	virtual bool stopAll(natural timeout = 0);
	virtual void join();
	virtual void finish();
	bool isRunning() const;

protected:

	class DynThread: public Thread, public DynObject {};

	typedef SharedPtr<DynThread> PThread;
	typedef AutoArray<PThread> ThreadPool;
	ThreadPool threadPool;
	ClusterAlloc alloc;
};



}





#endif /* LIGHTSPEED_ACTIONS_INFPARALLELEXECUTOR_H_ */
