/*
 * threadSleeper.h
 *
 *  Created on: 17.6.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_ITHREADSLEEPER_H_
#define LIGHTSPEED_MT_ITHREADSLEEPER_H_



#include "../base/memory/refCntPtr.h"
#include "timeout.h"
#include "platform.h"
#include "sleepingobject.h"

namespace LightSpeed {


	///Makes thread's sleeping object ref-counted reference
	class ISleepingObjectRefCnt: public ISleepingObject, public RefCntObj {
	public:
		ISleepingObjectRefCnt() {enableMTAccess();}
	};

	typedef ISleepingObjectRefCnt IThreadSleeper;

	///essencial object of every thread
	/** This is platform depend object which is used to implement sleep/wakeUp synchronization
	 * system. It implements ISleepingObject, so threads can be directly controlled by it.
	 *
	 * You can create an instance of this class anywhere, it works as "Windows Event" object with
	 * autoreset feature. Note that once your thread stops on this object, it will not respond
	 * to wakeup calls send to the thread object
	 */
	class ThreadSleeper;

}


#if defined(LIGHTSPEED_PLATFORM_WINDOWS)
#include "windows/threadSleeper.h"
#elif defined(LIGHTSPEED_PLATFORM_LINUX)
#include "linux/threadSleeper.h"
#else
#error unimplemented feature
#endif


#endif /* LIGHTSPEED_MT_THREADSLEEPER_H_ */
