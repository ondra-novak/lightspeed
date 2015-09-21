/*
 * dispatchThread.cpp
 *
 *  Created on: 15. 9. 2015
 *      Author: ondra
 */

#include "dispatcher.h"
#include "exceptions/dispatcher.h"
#include "../base/exceptions/canceledException.h"
#include "threadMinimal.h"
#include "../base/sync/threadVar.h"
#include "../base/actions/abstractDispatcher.tcc"




namespace LightSpeed {



Dispatcher::Dispatcher():queue(0),allocator(&StdAlloc::getInstance()),observer(getCurThreadSleepingObj()) {
	// TODO Auto-generated constructor stub

}

Dispatcher::Dispatcher(IRuntimeAlloc* allocator):queue(0),allocator(allocator),observer(getCurThreadSleepingObj()) {
}

Dispatcher::Dispatcher(ISleepingObject* observer):queue(0),allocator(&StdAlloc::getInstance()),observer(observer) {
}

Dispatcher::Dispatcher(ISleepingObject* observer,
		IRuntimeAlloc* allocator):queue(0),allocator(allocator),observer(observer) {
}

void Dispatcher::cancelQueue() {
	cancelAllPromises();
	while (queue) {
		AbstractAction *x = queue;
		queue = queue->next;
		x->reject(CanceledException(THISLOCATION));
		delete x;
	}
}

Dispatcher::~Dispatcher() {
	cancelQueue();
}

void Dispatcher::dispatchAction(AbstractAction* action) {
	AbstractAction *q = queue;
	do {
		action->next = q;
		q = lockCompareExchangePtr(&queue,action->next,action);
	} while (q != action->next);
	observer->wakeUp(0);

}

bool Dispatcher::implSleep(const Timeout& tm, natural& reason) {
	bool x = Thread::sleep(tm,reason);
	if (!x) executeQueue();
	return x;
}

void Dispatcher::executeQueue() {
	AbstractAction *q = 0;
	q = lockExchangePtr(&queue,q);
	if (q) {
		AbstractAction *rv = 0;
		while (q) {
			AbstractAction *z = q;
			q = q->next;
			z->next = rv;
			rv = z;
		}

		while (rv) {
			AbstractAction *z = rv;
			rv = rv->next;
			z->run();
			delete z;
		}
	}
}

bool Dispatcher::sleep(const Timeout& tm, natural& reason) {
	return implSleep(tm,reason);
}

bool Dispatcher::sleep(const Timeout& tm) {
	natural reason;
	return implSleep(tm,reason);
}

IRuntimeAlloc& Dispatcher::getActionAllocator() {
	return *allocator;
}


DispatcherThread::DispatcherThread():Dispatcher(this),alertable(true) {
}

DispatcherThread::DispatcherThread(IRuntimeAlloc* allocator):Dispatcher(this,allocator),alertable(true) {
}

void DispatcherThread::dispatchAction(AbstractAction *action)
{
	if (!isRunning()) {
		try {
			start(ThreadFunction::create(this, &DispatcherThread::run));
		}
		catch (ThreadBusyException &) {

		}
	}
	Dispatcher::dispatchAction(action);
}

void DispatcherThread::run() {
	while (!this->canFinish()) {
		natural x;
		this->impSleep(nil,x);
	}
}

bool DispatcherThread::enable(bool state) {
	bool prev = alertable;
	alertable = state;
	return prev;
}

bool DispatcherThread::impSleep(const Timeout& tm, natural& reason) {
	if (alertable) {
		alertable = false;
		bool x = Thread::impSleep(tm,reason);
		if (!x) executeQueue();
		alertable = true;
		return x;
	} else {
		return Thread::impSleep(tm,reason);
	}
}

} /* namespace LightSpeed */
