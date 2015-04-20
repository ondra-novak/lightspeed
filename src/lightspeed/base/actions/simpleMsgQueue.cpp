/*
 * simpleMessageQueue.cpp
 *
 *  Created on: 23.3.2015
 *      Author: ondra
 */


#include "simpleMsgQueue.h"
#include "../sync/synchronize.h"

namespace LightSpeed {

void SimpleMsgQueue::execute(const IExecAction& action) {
	Synchronized<FastLock> _(insertLock);
	actionList.add(ExecAction(action));
	onMessage();
}

bool SimpleMsgQueue::stopAll(natural timeout) {
	finish();
	if (timeout) join();
	return !isRunning();
}

void SimpleMsgQueue::finish() {
	Synchronized<FastLock> _(insertLock);
	actionList.clear();
}

void SimpleMsgQueue::join() {
	Synchronized<FastLock> _(execLock);
	//just acquire lock, do nothing
}

bool SimpleMsgQueue::isRunning() const {
	bool res = execLock.tryLock();
	if (res) {
		execLock.unlock();
	}
	return res;
}

bool SimpleMsgQueue::pumpMessage() {
	Synchronized<FastLock> _(insertLock);
	if (actionList.empty()) return false;
	ExecAction a = actionList.getFirst();
	actionList.eraseFirst();
	SyncReleased<FastLock> __(insertLock);
	Synchronized<FastLock> ___(execLock);
	a.deliver();
	return true;
}

bool SimpleMsgQueue::pumpAllMessages() {
	bool res = false;
	while (pumpMessage()) {
		res = true;
	}
	return res;
}

void SimpleMsgQueue::executePriority(const IExecAction& action) {
	Synchronized<FastLock> _(insertLock);
	actionList.insertFirst(ExecAction(action));
	onMessage();
}

bool SimpleMsgQueue::pumpMessageMT() {
	Synchronized<FastLock> _(insertLock);
	if (actionList.empty()) return false;
	ExecAction a = actionList.getFirst();
	actionList.eraseFirst();
	SyncReleased<FastLock> __(insertLock);
	a.deliver();
	return true;
}

void SimpleMsgQueue::onMessage() {}

}
