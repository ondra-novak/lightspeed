/*
 * threadMinimal.h
 *
 *  Created on: 18.10.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_THREADMINIMAL_H_
#define LIGHTSPEED_MT_THREADMINIMAL_H_

#include "atomic_type.h"
#include "threadSleeper.h"


namespace LightSpeed {


	class ISleepingObject;
	class Thread;
	class Fiber;
	class Gate;
	class Timeout;




	///Retrieves current thread's sleeping object
	/**
	 * @return sleeping object of the current thread. It can be very useful, if current thread
	 * need to register self for a notify. This function helps to achieve this without need to include
	 * whole lightspeed/mt section.
	 *
	 *	Function never returns NULL. It throws exception if current thread has no Thread associated
	 */
	RefCntPtr<IThreadSleeper> getCurThreadSleepingObj();

	///Retrieves current thread handle
	/**
	 * @return pointer to current thread.
	 */
	Thread *getCurrentThread();

	///Retrieves current fiber handle
	Fiber *getCurrentFiber();

	///Retrieves pointer to Gate object which becomes signaled when current thread exits
	Gate *getJoinObject();

	///Retrieves atomic number of the current thread
	atomicValue getCurThreadId();

	///Halts current thread until notification (through sleeping object) arrives.
	void threadHalt() throw();

	///Halts current thread until notification (through sleeping object) arrives. Thou can specify timeout
	/**
	 * @param interval Timeout interval
	 * @retval true timeout
	 * @retval false notification arrived
	 */
	bool threadSleep(const Timeout &interval) throw();

	///Returns true, when current thread can finish as soon as possibke
	/**
	 * @retval true current thread should finish
	 * @retval false current thread can continue
	 */
	bool threadCanFinish() throw();

	natural fiberHalt();

}


#endif /* LIGHTSPEED_MT_THREADMINIMAL_H_ */
