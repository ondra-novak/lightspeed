/*
 * threadSleeper.h
 *
 *  Created on: 17.6.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_LINUX_MT_THREADSLEEPER_H_
#define LIGHTSPEED_LINUX_MT_THREADSLEEPER_H_

#include <semaphore.h>

#include "../threadSleeper.h"


namespace LightSpeed {

	class ThreadSleeper: public IThreadSleeper {
	public:

		ThreadSleeper();
		~ThreadSleeper();
		natural getReason() const;
		void wakeUp(natural reason = 0) throw();
		bool sleep(Timeout tm);

	protected:
		sem_t semaphore;
		natural reason;


	};

}


#endif /* LIGHTSPEED_LINUX_MT_THREADSLEEPER_H_ */
