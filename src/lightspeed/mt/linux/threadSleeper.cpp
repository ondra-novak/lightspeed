/*
 * threadSleeper.cpp
 *
 *  Created on: 17.6.2013
 *      Author: ondra
 */


#include "threadSleeper.h"
#include <errno.h>
#include "../../base/exceptions/systemException.h"
#include "atomic.h"

namespace LightSpeed {


ThreadSleeper::ThreadSleeper():reason(0) {
	sem_init(&semaphore,0,0);
	enableMTAccess();
}

natural ThreadSleeper::getReason() const {
	return reason;
}

void ThreadSleeper::wakeUp(natural reason ) throw() {
	int val = 0;
	this->reason = reason;
	sem_getvalue(&semaphore,&val);
	if (val < 1) sem_post(&semaphore);

}

ThreadSleeper::~ThreadSleeper() {
	sem_destroy(&semaphore);
}

bool ThreadSleeper::sleep(Timeout timeout) {
	if (timeout.isInfinite()) {
		sem_wait(&semaphore);
	} else {
		timespec tmspc = timeout.getExpireTime().getTimeSpec();
		int res = sem_timedwait(&semaphore,&tmspc);
		if (res == -1) {
			int e = errno;
			if (e == ETIMEDOUT) return true;
			if (e != EINTR) throw ErrNoException(THISLOCATION,e);
			//Consider EINTR as wakeUp
			return false;
		}
	}
	while (sem_trywait(&semaphore) == 0) {}
	return false;
}

}
