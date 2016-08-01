/*
 * queueExecutor.cpp
 *
 *  Created on: 13. 7. 2016
 *      Author: ondra
 */


#include "queueExecutor.h"

#include "../../mt/thread.h"
#include "../sync/synchronize.h"
#include "../containers/queue.tcc"

namespace LightSpeed {

void QueueExecutor::wakeThread() {
	//try lock will lock semaphore if unlocked, otherwise keep locked
	//this prevents to accumulate counter value if semaphore is already unlocked
	semaphore.tryLock();
	//now unlock semaphore to have counter = 1, now any thread can continue to grab action from the queue
	semaphore.unlock();
}

void QueueExecutor::execute(const IExecAction& action) {
	//lock internals
	Synchronized<FastLock> _(lock);
	//if finish flag active, reject request
	if (finishFlag) return;

	//clone the action
	SharedPtr<IExecAction> a = action.clone();
	//push to the queue
	queue.push(a);
	//wake possible sleeping thread
	wakeThread();
}

QueueExecutor::QueueExecutor():semaphore(1),runningMessages(0),threadsIn(0) {

}

///handles entering and exitting from the serving thread
class QueueExecutor::Server {
public:

	///enteringing to the serving thread
	Server(QueueExecutor *owner) throw();
	///exiting from the serving thread
	~Server() throw();
	///server function
	void serve();


protected:
	QueueExecutor *owner;
};

QueueExecutor::Server::Server(QueueExecutor *owner) throw()
	:owner(owner) {
	//lock internals
	Synchronized<FastLock> _(owner->lock);

	//increase count of serving threads
	owner->threadsIn++;
	//close gate "noThreads"
	owner->noThreads.close();
}

QueueExecutor::Server::~Server() throw() {
	//lock internals
	Synchronized<FastLock> _(owner->lock);
	//decrement count of serving threads
	owner->threadsIn--;
	//if count of serving thread is zero
	if (owner->threadsIn == 0)
		//open gate "noThreads"
		owner->noThreads.open();
	else
		//wake another possible sleeping thread to re-check the queue status.
		owner->wakeThread();
}

void QueueExecutor::Server::serve() {

	while(true) {

		{
			//lock internals
			Synchronized<FastLock> _(owner->lock);
			//if finish flag or thread is marked finish, break
			if (owner->finishFlag || Thread::canFinish()) break;
			//while queue is not empty
			while (!owner->queue.empty()) {
				//pick next action
				SharedPtr<IExecAction> action = owner->queue.top();
				//remove it from queue
				owner->queue.pop();
				//increase running actions
				owner->runningMessages++;
				try
				{
					//unlock the lock temporary
					SyncReleased<FastLock> _(owner->lock);
					//perform action (in case of exception, continue after catch
					(*action)();
					//decrease running actions
					owner->runningMessages--;
				} catch (...) {
					//in case of exception, decrease running actions
					owner->runningMessages--;
					//throw above
					throw;
				}
				//if finish reported or thread will finish, break
				if (owner->finishFlag || Thread::canFinish()) break;
			}
		}
		//counts idle cycles
		natural counter=0;

		//if queue is empty, we need to wait for next message
		//when message is added, semaphore is unlocked
		//we can wait for infinite time, but it is better to give
		//implementator chance to control his thread. Call onIdle repeatedly
		bool contWait;
		do {
			//call onIdle and determine timeout (default infinite)
			Timeout tm = owner->onIdle(counter++);
			//wait for unlock semaphore and determine whether it succeed
			contWait = !owner->semaphore.lock(tm);
			//repeat if not
		} while (contWait);
	}
}


void QueueExecutor::serve() {
	//create server
	Server server(this);
	//run server
	server.serve();
}

bool LightSpeed::QueueExecutor::stopAll(natural timeout) {
	//finish all threads
	finish();
	//wait for everyone leave
	return noThreads.wait(timeout);

}

Timeout  QueueExecutor::onIdle(natural ) {
	//return infinity
	return Timeout(null);
}

void QueueExecutor::finish() {
	Synchronized<FastLock> _(lock);
	//clear queue
	queue.clear();
	//mark finished
	finishFlag=true;
	//wake up possible sleeping thread to receive finish flag status
	wakeThread();

}
void QueueExecutor::join() {
	//join is equal to stopAll
	stopAll(naturalNull);
}

QueueExecutor::~QueueExecutor() {
	//join all threads
	join();
	//ensure that nothing is running there and protect the destructor until lock is destroyed
	lock.lock();
}

bool QueueExecutor::isRunning() const {
	//function must return true, if there are running messages
	return runningMessages > 0;
}

void QueueExecutor::reset() {
	//reset finish flag
	finishFlag = false;
}

}
