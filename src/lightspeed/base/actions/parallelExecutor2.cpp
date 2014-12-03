/*
 * parallelExecutor2.cpp
 *
 *  Created on: 16.4.2013
 *      Author: ondra
 */

#include "parallelExecutor2.h"
#include "../../mt/atomic.h"
#include "../../mt/thread.h"
#include "../../mt/exceptions/timeoutException.h"
#include "../memory/allocPointer.h"
#include "../exceptions/stdexception.h"
#include "../actions/parallelExecutor.h"

namespace LightSpeed {

ParallelExecutor2::ParallelExecutor2(natural maxThreads,natural maxWaitTimeout,
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

ParallelExecutor2::ParallelExecutor2(const ParallelExecutor2& other)
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

ParallelExecutor2::~ParallelExecutor2() { try {
	stopAll(naturalNull);
} catch (...) {
	if (!std::uncaught_exception()) throw;
} }

class ParallelExecutor2::Worker {
	public:

	///initializes worker
	/**
	 * @param owner reference to executor
	 * @param next pointer to current top worker
	 */
	Worker(ParallelExecutor2 &owner, Worker *next):owner(owner),nextWorker(next) {}

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
	ParallelExecutor2 &owner;
	Pointer<Worker> nextWorker;
	PException lastException;
	Thread thr;

	friend class ParallelExecutor2;

	void runAction(const Message<void>::Ifc *action);
};

void ParallelExecutor2::cleanUp() {
	//perform maintaince cleanup
	while (topWorker != nil && topWorker->isDead()) {
		Pointer<Worker> x = topWorker;
		topWorker = x->nextWorker;
		x->thr.join();
		delete x.get();
	}
}

void ParallelExecutor2::checkException() {
	if (lastException != nil) {
		PException e = lastException;
		lastException = 0;
		e->throwAgain(THISLOCATION);
	}
}

void ParallelExecutor2::execute(const IExecAction &action) {

	orderStop = false;

	checkException();
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
	if (waitPt.notifyOne()) {
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





	//notify any waiting thread, if there is such
/*	bool threadInPool = waitPt.notifyOne();
	try {
		if (threadInPool) {
			//pool confirmed that thread has been released from pool
			//wait for him infinite time
			while (!ntf) Thread::sleep(naturalNull);
		} else {

			bool tryCreateThread = true;
			//cleanup all dead workers on top
			cleanUp();
			//maxWait - how long caller will wait
			Timeout maxWait(maxWaitTimeout);
			//curWait - how long caller will wait to create new threadh
			Timeout curWait(newThreadTimeout);
			//in cases no running threads, start first one
			if (topWorker == 0) {
				startNewThread();
				tryCreateThread = false;
				curWait = maxWait;
			}
			bool ntfst, actst;
			//repeat while ntf is not signaled or there is still message
			//(thread must take message and notify caller -
			// - anything other is false alarm causing continue to wait
			ntfst = ntf;
			actst = readAcquirePtr(&curAction) == 0;
			while (!ntfst || !actst) {
				//if not notified
				if (!ntfst) {
					//check action,
					if (actst) {
						//action given, we have to set timeout to infinite
						//because thread will soon on later send notification
						curWait = Timeout(naturalNull);
					} else if (curWait.expired()) {
						if (tryCreateThread) {
							//start new thread
							startNewThread();
							tryCreateThread = false;
							curWait = maxWait;
						} else  {
							//report timeout
							throw TimeoutException(THISLOCATION);
						}
					}
				} else {
					//in case that notification without action taken
					//special notification from last thread while there is still action
					//reset notification
					ntf.reset();
					//perform maintaince cleanup
					cleanUp();
					//check whether there is thread to process message
					if (topWorker == 0) {
						//and start new thread
						startNewThread();
					}
				}
				//wait for notification
				Thread::sleep(curWait);
				ntfst = ntf;
				actst = readAcquirePtr(&curAction) == 0;

			}
		}
		//unregister caller
		writeReleasePtr<ISleepingObject>(&callerNtf,0);
		//unregister action
		writeReleasePtr<const ExecAction>(&curAction,0);
	} catch (...) {
		//unregister caller
		writeReleasePtr<ISleepingObject>(&callerNtf,0);
		//unregister action
		writeReleasePtr<const ExecAction>(&curAction,0);
		throw;
	}*/

}

void ParallelExecutor2::Worker::runAction(const IExecAction *action) {
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
	owner.lastException = lastException;
	//notify caller, action taken, worker starting to work
	owner.callerNtf->wakeUp(0);
	try {
		const IExecAction &proc = *a;
		//execute action
		proc();
	} catch (Exception &x) {
		//store exception
		lastException = x.clone();
	} catch (std::exception &e) {
		//store exception
		lastException = new StdException(THISLOCATION, e);
	} catch (...) {
		//store exception
		lastException = new UnknownException(THISLOCATION);
	}

}

void ParallelExecutor2::Worker::run() {
	bool cont = true;

	owner.onThreadInit();
	//repeat until thread finish
	while (cont) {
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
				if (!owner.waitPt.wait(slot,tm,SyncPt::interruptOnExit)) {
					//timeout - my thread will finish - decrease count of threads
					lockDec(owner.curThreadCount);
					//when timeout - remove slot
					owner.waitPt.remove(slot);
					//slot becomed signaled meanwhile
					if (slot) {
						//if signaled, thread remain active, return number back
						lockInc(owner.curThreadCount);
					} else {
						//leave loop
						cont = false;
					}
				}
				//count idles
				lockDec(owner.idleCount);
			}
		}

		cleanUp();

		integer p = (integer)owner.curThreadCount;
		if (p > (integer)owner.maxThreads && lockCompareExchange(owner.curThreadCount,p,p-1) == p) {
			break;
		}
	}

	owner.onThreadDone();
}

bool ParallelExecutor2::stopAll(natural timeout) {
	Timeout tm(timeout);
	orderStop = true;
	waitPt.notifyAll();
	while (topWorker != nil) {
		if (!topWorker->stop(timeout)) return false;
		cleanUp();
	}
	checkException();
	return true;
}

void ParallelExecutor2::setMaxThreadCount(natural count) {
	if (count == 0) count = ParallelExecutor::getCPUCount();
	if (count == 0) count = 1;
	maxThreads = count;
	waitPt.notifyAll();
}

natural ParallelExecutor2::getMaxThreadCount() {
	return maxThreads;
}

void ParallelExecutor2::spawnThreads(natural count) {
	while ((natural)curThreadCount < maxThreads && count) {
		startNewThread();
		count--;
	}
}

natural ParallelExecutor2::getThreadCount() const {
	return curThreadCount;
}

void ParallelExecutor2::setWaitTimeout(natural tm) {
	maxWaitTimeout = tm;
}

void ParallelExecutor2::setNewThreadTimeout(natural tm) {
	newThreadTimeout = tm;
}

void ParallelExecutor2::setIdleTimeout(natural tm) {
	threadIdleTimeout = tm;
}

natural ParallelExecutor2::getWaitTimeout() const {
	return maxWaitTimeout;
}

natural ParallelExecutor2::getNewThreadTimeout() const {
	return newThreadTimeout;
}

natural ParallelExecutor2::getIdleTimeout() const {
	return threadIdleTimeout;
}

natural ParallelExecutor2::getCPUCount() {
	return ParallelExecutor::getCPUCount();
}

void ParallelExecutor2::startNewThread() {
	if ((natural)curThreadCount < maxThreads) {
		AllocPointer<Worker> w ( new Worker(*this,topWorker));
		w->start();
		topWorker = w.detach();
		lockInc(curThreadCount);
	}
}


inline bool ParallelExecutor2::Worker::isDead() const {
	return !thr.isRunning();
}

void ParallelExecutor2::Worker::cleanUp() {
	while (nextWorker && !nextWorker->thr.isRunning()) {
		Pointer<ParallelExecutor2::Worker> x = nextWorker;
		nextWorker = x->nextWorker;
		x->thr.join();
		delete x;
	}

}

inline void ParallelExecutor2::Worker::start() {
	thr.start(ThreadFunction::create(this,&ParallelExecutor2::Worker::run));
}

inline bool ParallelExecutor2::Worker::stop(const Timeout& tm) {
	thr.finish();
	return thr.getJoinObject().wait(tm,SyncPt::uninterruptible);
}

void ParallelExecutor2::finish() {
}

} /* namespace LightSpeed */

