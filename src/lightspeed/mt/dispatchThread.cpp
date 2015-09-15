/*
 * dispatchThread.cpp
 *
 *  Created on: 15. 9. 2015
 *      Author: ondra
 */

#include "dispatchThread.h"
#include "../base/exceptions/canceledException.h"

namespace LightSpeed {

DispatchThread::DispatchThread():queue(0),allocator(&StdAlloc::getInstance()) {
	// TODO Auto-generated constructor stub

}

DispatchThread::~DispatchThread() {
	cancelAllPromises();
	while (queue) {
		AbstractAction *x = queue;
		queue = queue->next;
		x->reject(CanceledException(THISLOCATION));
		delete x;
	}
}

void DispatchThread::dispatchAction(AbstractAction* action) {
	AbstractAction *q = queue;
	do {
		action->next = q;
		q = lockCompareExchangePtr(&queue,action->next,action);
	} while (q != action->next);
	wakeUp(0);
}

bool DispatchThread::impSleep(const Timeout& tm, natural& reason) {
	bool x = Thread::impSleep(tm,reason);
	if (!x) executeQueue();
	return x;
}

void DispatchThread::executeQueue() {
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

DispatchThread::DispatchThread(IRuntimeAlloc* allocator):allocator(allocator) {
}

IRuntimeAlloc& DispatchThread::getActionAllocator() {
	return *allocator;
}

} /* namespace LightSpeed */
