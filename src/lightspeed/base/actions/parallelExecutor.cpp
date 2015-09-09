/*
 * parallelExecutor2.cpp
 *
 *  Created on: 16.4.2013
 *      Author: ondra
 */

#include "parallelExecutor.h"
#include "../../mt/atomic.h"
#include "../../mt/thread.h"
#include "../../mt/exceptions/timeoutException.h"
#include "../memory/allocPointer.h"
#include "../exceptions/stdexception.h"
#include "../actions/parallelExecutor.h"
#include "../framework/iapp.h"

namespace LightSpeed {

ParallelExecutor::ParallelExecutor(natural maxThreads,natural maxWaitTimeout,
	 	  natural newThreadTimeout,natural threadIdleTimeout)
	:maxThreads(maxThreads)
	,maxWaitTimeout(maxWaitTimeout)
	,newThreadTimeout(newThreadTimeout)
	,threadIdleTimeout(threadIdleTimeout)
	,curThreadCount(0)
	,idleCount(0)
	,curAction(0)
	,waitPt(SyncPt::stack)
	,orderStop(false)
{
	if (this->maxThreads == 0) this->maxThreads = ParallelExecutor::getCPUCount();
	if (this->maxThreads == 0) this->maxThreads = 1;
}

ParallelExecutor::ParallelExecutor(const ParallelExecutor& other)
:maxThreads(other.maxThreads)
,maxWaitTimeout(other.maxWaitTimeout)
,newThreadTimeout(other.newThreadTimeout)
,threadIdleTimeout(other.threadIdleTimeout)
,curThreadCount(0)
,idleCount(0)
,curAction(0)
,waitPt(SyncPt::stack)
,orderStop(false)
{
}

ParallelExecutor::~ParallelExecutor() { try {
	stopAll(naturalNull);
} catch (...) {
	if (!std::uncaught_exception()) throw;
} }

class ParallelExecutor::Worker {
	public:

	///initializes worker
	/**
	 * @param owner reference to executor
	 * @param next pointer to current top worker
	 */
	Worker(ParallelExecutor &owner, Worker *next):owner(owner),nextWorker(next) {}

	///worker's execution function
	void run();
	///determines worker state
	/**
	 * @retval true worker is dead and can be cleaned
	 * @retval false worker is not dead
	 */
	bool isDead() const;

	void cleanUp();
	///starts worker's thread
	void start();
	///stops worker's thread
	/**
	 * @oaram tm timeout
	 * @retval true ok
	 * @retval false timeout
	 */
	bool stop(const Timeout &tm);
protected:
	ParallelExecutor &owner;
	Pointer<Worker> nextWorker;
	Thread thr;

	friend class ParallelExecutor;

	void runAction(const Message<void>::Ifc *action);
};

void ParallelExecutor::cleanUp() {
	//perform maintaince cleanup
	//check just only topWorker
	//no synchronization needed because only this thread can manage the top worker
	while (topWorker != nil && topWorker->isDead()) {
		Pointer<Worker> x = topWorker;
		//remove top worker
		topWorker = x->nextWorker;
		x->thr.join();
		//delete dead instance
		delete x.get();
	}
}


void ParallelExecutor::execute(const IExecAction &action) {

	Synchronized<FastLock> _(executeLock);

	orderStop = false;

	//create notifier
	Notifier ntf;
	//register notifier
	writeReleasePtr<ISleepingObject>(&callerNtf,&ntf);
	//write new action
	writeReleasePtr<const IExecAction>(&curAction,&action);

	//true - create new thread after timeout
	bool ct = true;

	//read system time
	SysTime curTime = SysTime::now();
	//setup max wait
	Timeout maxWait(curTime,maxWaitTimeout);
	//setup wait for creation of new thread
	Timeout curWait(curTime,newThreadTimeout);
	//cleanup any dead threads
	cleanUp();

	//notify wait sync point to release one thread
	if (wakeThread()) {
		//if success, at least one thread MUST take a message - disable new thread creation
		ct = false;
		//use maxWait as timeout
		curWait = maxWait;
	}

	bool cont = true;
	//until notifier is signaled
	while (cont) {
		//whether current wait expired
		if (curWait.expired(curTime)) {
			//we still did not try to create new thread
			if (ct) {
				//start new one (function will limit to maximum threads)
				startNewThread();
				//we tried, reset flag
				ct = false;
				//use maxWait as timeout
				curWait = maxWait;
			} else {
				//timeout while waiting - remove message
				const IExecAction * actst = lockExchangePtr<const IExecAction>(&curAction,0);
				//message cannot be removed, another thread already reading it
				if (actst == 0) {
					//set waiting to infinite - thread should send signal as soon as possible
					curWait = Timeout(naturalNull);
					//continue waiting
				} else {
					//release notifier
					writeReleasePtr<ISleepingObject>(&callerNtf,0);
					//throw exception
					throw TimeoutException(THISLOCATION);
				}
			}
		}
		//cleanup any dead threads
		cleanUp();
		//if there is no thread
		if (topWorker == 0 || curThreadCount == 0) {
			//create one!
			startNewThread();
			//thread created
			ct = false;

			curWait = maxWait;
		}
		//now sleep and wait for event
		cont = !ntf.wait(curWait,false);
		//read current time;
		curTime = SysTime::now();

	}
	//message delivered, release notifier
	writeReleasePtr<ISleepingObject>(&callerNtf,0);

}

void ParallelExecutor::Worker::runAction(const IExecAction *action) {
	//caller still locked, so action pointer is valid
	//calculate size of action i bytes
	natural sz = action->getObjectSize();
	//allocate buffer on stack
	void *buff = alloca(sz);
	//create allocator
	AllocInBuffer abuff(buff,sz);
	//clone action into the buffer in the stack
	AllocPointer<IExecAction> a(action->clone(abuff));
	//store last exception
	//notify caller, action taken, worker starting to work
	owner.callerNtf->wakeUp(0);
	try {
		const IExecAction &proc = *a;
		//execute action
		proc();
	} catch (Exception &x) {
		IApp::threadException(x);
	} catch (std::exception &e) {
		IApp::threadException(StdException(THISLOCATION, e));
	} catch (...) {
		IApp::threadException(UnknownException(THISLOCATION));
	}

}

void ParallelExecutor::Worker::run() {
	bool cont = true;

	//perform thread-init hook
	owner.onThreadInit();
	//repeat until thread finish
	while (cont && !Thread::canFinish() && !owner.orderStop) {
		//retrieve action - make action unaccessible for others
		const IExecAction * action = lockExchangePtr<const IExecAction >(&owner.curAction,(IExecAction *)0);
		//is there action?
		if (action != 0) {
			//run action
			runAction(action);
		} else {
			//no action retrieved, we have to wait on action
			SyncPt::Slot slot;
			//register self to syncpt
			owner.waitPt.add(slot);


			//check action again
			action = lockExchangePtr<const IExecAction>(&owner.curAction,(IExecAction *)0);
			//if there is action ...
			if (action != 0) {
				//this happens, when action appears before we
				//were registered.
				//remove self from the syncpt
				owner.waitPt.remove(slot);
				//run action
				runAction(action);
			} else {
				//now we are registered, no action pending
				//so caller will wake registered slot after it post action
				//prepare timeout
				Timeout tm(owner.threadIdleTimeout);
				//count idles
				lockInc(owner.idleCount);
				//callback on idle
				owner.onThreadIdle(slot.getNotifier());
				//wait for timeout
				if (!owner.waitPt.wait(slot,tm,SyncPt::interruptOnExit)
						//when timeout - remove slot
						//but it may fail, because signal arrived later
						&& owner.waitPt.remove(slot)) {
					//timeout - my thread will finish - decrease count of threads
					lockDec(owner.curThreadCount);
					//recheck whether there is action arrived meanwhile
					action = lockExchangePtr<const IExecAction >(&owner.curAction,(IExecAction *)0);
					//after leaving waiting point but before count of running threads is decreased
					//a new action can arrived causing failure of the creation of the new
					//thread especially, when maxThreadCount is set to low value
					//This can cause deadlock. So after worker decreased curThreadCount, it must
					//perform extra check for the action. If there is an action, it has been probably
					//queued during the worker was leaving wait point and before count has been decreased
					//There is also possibility, that it happened after so new extra thread may appear
					//in the pool.
					if (action) {
						//increase thread count - I am alive again
						lockInc(owner.curThreadCount);
						//count idles - I am working
						lockDec(owner.idleCount);
						//run action
						runAction(action);
						//count idles - I am not working now
						lockInc(owner.idleCount);
					} else {
						//now we can safety leave the loop, because this thread is no more counted
						//leave loop
						cont = false;
					}

				}
				//count idles - I am working now
				lockDec(owner.idleCount);
			}
		}

		//cleanup any other finished threads
		cleanUp();

		//check current thread count
		integer p = (integer)owner.curThreadCount;
		//if current thread count is above limit and we successfully set it one less lower
		if (p > (integer)owner.maxThreads && lockCompareExchange(owner.curThreadCount,p,p-1) == p) {
			//exit this thread uncoditionally
			break;
		}
	}

	//perform thread-done hook
	owner.onThreadDone();
}

bool ParallelExecutor::stopAll(natural timeout) {

	Synchronized<FastLock> _(executeLock);

	Timeout tm(timeout);
	orderStop = true;
	waitPt.notifyAll();
	while (topWorker != nil) {
		if (!topWorker->stop(timeout)) return false;
		cleanUp();
	}
	return true;
}

void ParallelExecutor::setMaxThreadCount(natural count) {
	if (count == 0) count = ParallelExecutor::getCPUCount();
	if (count == 0) count = 1;
	maxThreads = count;
	waitPt.notifyAll();
}

natural ParallelExecutor::getMaxThreadCount() {
	return maxThreads;
}

void ParallelExecutor::spawnThreads(natural count) {
	while ((natural)curThreadCount < maxThreads && count) {
		startNewThread();
		count--;
	}
}

natural ParallelExecutor::getThreadCount() const {
	return curThreadCount;
}

void ParallelExecutor::setWaitTimeout(natural tm) {
	maxWaitTimeout = tm;
}

void ParallelExecutor::setNewThreadTimeout(natural tm) {
	newThreadTimeout = tm;
}

void ParallelExecutor::setIdleTimeout(natural tm) {
	threadIdleTimeout = tm;
}

natural ParallelExecutor::getWaitTimeout() const {
	return maxWaitTimeout;
}

natural ParallelExecutor::getNewThreadTimeout() const {
	return newThreadTimeout;
}

natural ParallelExecutor::getIdleTimeout() const {
	return threadIdleTimeout;
}

void ParallelExecutor::startNewThread() {
	if ((natural)curThreadCount < maxThreads) {
		AllocPointer<Worker> w ( new Worker(*this,topWorker));
		w->start();
		topWorker = w.detach();
		lockInc(curThreadCount);
	}
}

bool ParallelExecutor::wakeThread() {
	return waitPt.notifyOne();
}


inline bool ParallelExecutor::Worker::isDead() const {
	return !thr.isRunning();
}

void ParallelExecutor::Worker::cleanUp() {
	//every worker has to cleanup it's "next" worker
	//check whether there is a next worker and whether it run
	while (nextWorker && nextWorker->isDead()) {
		//if there is dead worker - pick pointer
		Pointer<ParallelExecutor::Worker> x = nextWorker;
		//remove that worker from the list
		nextWorker = x->nextWorker;
		//join worker to clear thread resources
		x->thr.join();
		//delete instance
		delete x;
		//continue to clean other workers in the list until lived-one found.
	}

}

inline void ParallelExecutor::Worker::start() {
	thr.start(ThreadFunction::create(this,&ParallelExecutor::Worker::run));
}

inline bool ParallelExecutor::Worker::stop(const Timeout& tm) {
	thr.finish();
	return thr.getJoinObject().wait(tm,SyncPt::uninterruptible);
}

void ParallelExecutor::finish() {
	orderStop = true;
}

} /* namespace LightSpeed */

