/*
 * threadMinimal.cpp
 *
 *  Created on: 18.10.2013
 *      Author: ondra
 */
#include "threadMinimal.h"
#include "thread.h"
#include "fiber.h"
#include "microlock.h"

namespace LightSpeed {

ISleepingObject* getCurThreadSleepingObj() {

	return &Thread::current();
}

Thread* getCurrentThread() {
	return &Thread::current();
}

/*
Fiber* getCurrentFiber() {
	return &Fiber::current();
}
*/

Gate* getJoinObject() {
	return &Thread::current().getJoinObject();
}

atomicValue getCurThreadId() {
	return ThreadId::current().asAtomic();
}

void threadHalt() throw() {
	Thread::sleep(nil);
}

bool threadSleep(const Timeout& interval) throw() {
	return Thread::sleep(interval);
}

bool threadCanFinish() throw() {
	return Thread::canFinish();
}

/*
natural fiberHalt() {
	return Fiber::sleep();
}
*/

}
