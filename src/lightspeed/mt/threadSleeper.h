/*
 * threadSleeper.h
 *
 *  Created on: 17.6.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_THREADSLEEPER_H_
#define LIGHTSPEED_MT_THREADSLEEPER_H_



#include "timeout.h"
#include "platform.h"
#if defined(LIGHTSPEED_PLATFORM_WINDOWS)
#include "windows/threadSleeper.h"
#elif defined(LIGHTSPEED_PLATFORM_LINUX)
#include "linux/threadSleeper.h"
#else
#error unimplemented feature
#endif

namespace LightSpeed {


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



#endif /* LIGHTSPEED_MT_THREADSLEEPER_H_ */
