/*
 * parallelExecutor2.h
 *
 *  Created on: 16.4.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_PARALLELEXECUTOR2_H_
#define LIGHTSPEED_BASE_PARALLELEXECUTOR2_H_

#include "executor.h"
#include "../../mt/slist.h"
#include "../../mt/syncPt.h"
#include "../../mt/atomic_type.h"
#include "../../mt/thread.h"

namespace LightSpeed {

class ParallelExecutor: public LightSpeed::IExecutor {
public:
	///Initialize executor
	/**
	 * @param maxThreads maximum thread count. If count is zero, function
	 *  uses count of CPUs
	 * @param maxWaitTimeout specifies count of milliseconds to wait for idle
	 * 	 worker.
	 * @param newThreadTimeout specifies timeout on idle worker after
	 * 	 a new thread is spawned. Default value 0 causes that new thread
	 * 	 is spawned for every action until maximum threads are reached. You
	 * 	 can specify higher value to slowdown rapid spawning of new threads
	 * 	 when several actions arrives in single block
	 * @param threadIdleTimeout specifies timeout after idle worker is
	 *   terminated.
	 */
	ParallelExecutor(natural maxThreads = 0,
			 	 	  natural maxWaitTimeout = naturalNull,
			 	 	  natural newThreadTimeout = 0,
			 	 	  natural threadIdleTimeout = naturalNull);

	virtual ~ParallelExecutor();

	///initializes executor using setup of another executor
	ParallelExecutor(const ParallelExecutor &other);

	///Executes action in thread pool
	/**
	 * @param action action to execute
	 * @exception TimeoutException time ellapsed during waiting on idle thread
	 * @note Function is not MT safe. If you want to call this function
	 * from different threads, use appropriate locking.
	 */
	virtual void execute(const IExecAction &action);

	///Orders are threads to stop
	/**
	 * @param timeout timeout in miliseconds.
	 * @retval true all stopped
	 * @retval false timeout.
	 * @note in case of timeout, order is not canceled. Order can
	 * be canceled by executing action or spawning new threads spawnThreads
	 */
	virtual bool stopAll(natural timeout = 0) ;


	///adjust max thread count
	/**
	 * Function can be called any-time and can be used to adjust current count of threads.
	 * Final count will be reached later. If more threads is requested, new threads will
	 * be started with new tasks. You can speedup this process calling function spawnThreads.
	 * If less threads is requested, function directs all extra threads to finish its work
	 * and terminate as soon as possible.
	 *
	 * @param count
	 */
	void setMaxThreadCount(natural count);

	///Retrieves current max count of threads
	natural getMaxThreadCount();

	///Starts threads
	/** By default, threads are started when task is not executed after `newThreadTimeout`
	 * Milliseconds. This field can be zero, which causes, that new thread is started right
	 * when there is no idle thread in time of executing task. This still take a small
	 * piece of time needed to create and start the thread. If you know, that there will  be
	 * a lot of work, you can spawn extra threads before executing the tasks.
	 * @param count specifies count of threads to spawn, Default value means that maximum
	 * possible threads (specified by maxThread) will be spawn.
	 */
	void spawnThreads(natural count = naturalNull);

	///Returns current thread count
	natural getThreadCount() const;


	///Specifies how long there will be waiting before operation fails with exception TimeoutException
	/**
	 * @param tm if value is naturalNull, the executor will wait infinite time for idle thread.
	 */
	void setWaitTimeout(natural tm);

	///sets new thread creation timeout
	/** If zero set, the new thread is created everytime the idle count is zero. If any number
	 * above zero is specified, the number defines count of milliseconds to wait before new
	 * thread is created.
	 * @param tm
	 */
	void setNewThreadTimeout(natural tm);

	///Sets count of miliseconds before idle thread is terminated
	/**
	 * @param tm you can specify naturalNull (default) and idle threads will never terminated (unless
	 * the executor destruction and stopAll function).
	 *
	 */
	void setIdleTimeout(natural tm);

	natural getWaitTimeout() const;

	natural getNewThreadTimeout() const;

	natural getIdleTimeout() const;

	///retrieves count of CPUs available on this computer
	static natural getCPUCount();

	///called when worker is initialized
	/** Function is empty, but descendant class can place a custom code here
	 *
	 *  @note Thread executing this code is counted as active. Executor will not create additional threads
	 *  when maximum count is reached.
	 */
	virtual void onThreadInit() {}

	///called before worker is destroyed
	/** Function is empty, but descendant class can place a custom code here
	 *
	 * @note while this function is executed, thread is no longer count as active. If there
	 * is longer code, executor can create additional threads until it reaches maximum count. Total
	 * count of threads can be higher than maximum. Don't place code here taking long time to run, or
	 * calculate with possibility that more threads can be created
	 */
	virtual void onThreadDone() {}

	///Called when thread has nothing to do
	/**
	 * @param ntf notifier which becomes signaled when new work is ready. Function should exit as soon
	 * as possible in this case.
	 *
	 * Leaving this function sooner causes that thread start sleeping and waiting for notification
	 */
	virtual void onThreadIdle(const Notifier &) {}


	natural getIdleCount() const {return idleCount;}

	bool isRunning() const {return idleCount < curThreadCount;}


	///TODO: finish not implemented yet.
	void finish();

	virtual void join() {stopAll(naturalNull);}

protected:
	natural maxThreads;
	natural maxWaitTimeout;
	natural newThreadTimeout;
	natural threadIdleTimeout;
	atomic curThreadCount;
	atomic idleCount;


	class Worker;
	friend class Worker;


	Pointer<Worker> topWorker;
	ISleepingObject * volatile callerNtf;
	const Message<void>::Ifc * volatile curAction;
	SyncPt waitPt;
	FastLock executeLock;
	bool orderStop;
	virtual void startNewThread();
	///Called when executor wants to wake one thread from the pool of the waiting threads
	/** Function tries to wake one thread
	  @retval true one thread woken up
	  @retval false thread pool is empty, there is no running thread, or all threads are busy

	  Descendant class can re-implement this function to introduce own way how to wake threads. For 
	  example if threads is waiting for other event and executor want to give them some work
	  */
	virtual bool wakeThread();


private:
	void cleanUp();
};

} /* namespace LightSpeed */
#endif /* LIGHTSPEED_BASE_PARALLELEXECUTOR2_H_ */
