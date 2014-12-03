/*
 * threadSleeper.cpp
 *
 *  Created on: 17.6.2013
 *      Author: ondra
 */

#include "winpch.h"
#include "threadSleeper.h"
#include "../../base/exceptions/systemException.h"

namespace LightSpeed {


ThreadSleeper::ThreadSleeper():reason(0) {
	semaphore = CreateEvent(0,0,0,0);
}

natural ThreadSleeper::getReason() const {
	return reason;
}

void ThreadSleeper::wakeUp(natural reason) throw() {
	this->reason = reason;
	SetEvent(semaphore);
}

ThreadSleeper::~ThreadSleeper() {
	CloseHandle(semaphore);
}

bool ThreadSleeper::sleep(Timeout timeout) {
	if (timeout.isInfinite()) {		
		DWORD res;
		do {
			res= WaitForSingleObject(semaphore,INFINITE);
			if (res == WAIT_FAILED) 
				throw ErrNoException(THISLOCATION,GetLastError());
		} while (res == WAIT_IO_COMPLETION);
		return false;
	} else {
		DWORD res;
		do {
			DWORD msecs = timeout.getRemain().msecs();
			res = WaitForSingleObjectEx(semaphore,msecs,TRUE);
			if (res == WAIT_FAILED) 
				throw ErrNoException(THISLOCATION,GetLastError());		
		} while (res == WAIT_IO_COMPLETION);
		return res == WAIT_TIMEOUT;
	}
}

}
