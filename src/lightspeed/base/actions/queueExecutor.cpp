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



void QueueExecutor::execute(const IExecAction& action) {

	Synchronized<FastLock> _(lock);
	if (finishFlag) return;

	SharedPtr<IExecAction> a = action.clone();
	queue.push(a);
	syncPt.notifyOne();
}

QueueExecutor::QueueExecutor():runningMessages(0),threadsIn(0) {

}

void QueueExecutor::serve() {
	{
		Synchronized<FastLock> _(lock);
		threadsIn++;
		noThreads.close();
	}
	natural counter=0;

	for(;;) {
		{
		Synchronized<FastLock> _(lock);
		if (finishFlag || Thread::canFinish()) break;
		if (!queue.empty()) {
			SharedPtr<IExecAction> action = queue.top();
			queue.pop();
			runningMessages++;
			{
				SyncReleased<FastLock> _(lock);
				(*action)();
				//TODO: handle an exception!
			}
			runningMessages--;
			counter=0;
		}
		if (finishFlag || Thread::canFinish()) break;
		}

		SyncPt::Slot slot;
		Timeout tm = onIdle(counter++);
		syncPt.add(slot);
		if (!syncPt.wait(slot,tm,SyncPt::interruptOnExit)) {
			syncPt.remove(slot);
		}
	}

	{
		Synchronized<FastLock> _(lock);
		threadsIn--;
		if (threadsIn == 0)
			noThreads.open();
		else
			syncPt.notifyOne();
	}
}

bool LightSpeed::QueueExecutor::stopAll(natural timeout) {
	finish();
	return noThreads.wait(timeout);

}

Timeout  QueueExecutor::onIdle(natural ) {
	return null;
}

void QueueExecutor::finish() {
	Synchronized<FastLock> _(lock);
	queue.clear();
	finishFlag=true;
	syncPt.notifyOne();
}

void QueueExecutor::join() {
	stopAll(naturalNull);
}

QueueExecutor::~QueueExecutor() {
	stopAll();
	//ensure that nothing is running there and protect the destructor until lock is destroyed
	lock.lock();
}

bool QueueExecutor::isRunning() const {
	return runningMessages > 0;
}

void QueueExecutor::reset() {
	finishFlag = false;
}

}
