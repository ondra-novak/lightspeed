/*
 * threadSleeper.h
 *
 *  Created on: 17.6.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_LINUX_MT_THREADSLEEPER_H_
#define LIGHTSPEED_LINUX_MT_THREADSLEEPER_H_
#include "../../base/types.h"
#include "../../mt/timeout.h"
#include "../../mt/sleepingobject.h"
#include "../../base/memory/refCntPtr.h"


namespace LightSpeed {

	class ThreadSleeper: public IThreadSleeper {
	public:

		ThreadSleeper();
		~ThreadSleeper();
		natural getReason() const;
		void wakeUp(natural reason = 0) throw();
		bool sleep(Timeout tm);


	protected:
		HANDLE semaphore;
		natural reason;


	};

}


#endif /* LIGHTSPEED_LINUX_MT_THREADSLEEPER_H_ */
