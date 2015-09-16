/*
 * microlock.h
 *
 *  Created on: 18.10.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_MICROLOCK_H_
#define LIGHTSPEED_MT_MICROLOCK_H_
#include "../base/sync/synchronize.h"
#include "atomic.h"
#include "threadMinimal.h"
#include "sleepingobject.h"


namespace LightSpeed {


	struct MicroLockData {
		MicroLockData *next;
		RefCntPtr<IThreadSleeper> owner;
	};


	///Full functional mutex/lock takes minimum of the memory and none of system resources
	/**
	 * MicroLock takes 4 bytes on 32bit platform and 8 bytes on 64bit platform and no system resources
	 * It is developed from the FastLock. To access the lock, you have to use Synchronize<MicroLock>
	 * That class is specialized to use with MicroLock. This is only way how to control this lock. There
	 * is no lock/unlock/trylock functions.
	 *
	 */
	class MicroLock {
		MicroLockData *owner;

		friend class Synchronized<MicroLock>;
	public:
		MicroLock():owner(0) {}
	};

	template<>
	class Synchronized<MicroLock, Empty>: public MicroLockData {
	public:


		Synchronized(MicroLock &lk):lk(lk) { lock();}
		~Synchronized() { unlock();}

	protected:
		MicroLock &lk;

		void lock() {
			//set next to 0, we want to have this as result
			next = 0;
			//set owner to current sleeping object
			owner = getCurThreadSleepingObj();

			//try to lock the lock, but if it is already locked, k will not NULL
			MicroLockData *k = lockCompareExchangePtr<MicroLockData>(&lk.owner,0,this);
			//test result of k
			while (k != next) {
				//update next to reflect reality
				next = k;
				//accept reality and try to enqueue self to the queue
				k = lockCompareExchangePtr<MicroLockData>(&lk.owner,next,this);
			}
			//wait until next is equal zero
			while (next != 0) {
				//halt thread and wait for notification
				threadHalt();
			}
		}

		void unlock() {
			//next must be zero for owned lock
			if (next == 0) {
				//try to unlock the lock complete
				MicroLockData *k = lockCompareExchangePtr<MicroLockData>(&lk.owner,this,0);
				//We need to have "this" in the k.
				//if not - the lock cannot be unlocked, we have to give ownership
				if (k != this) {
					//find next object in the queue
					while (k->next != this) k = k -> next;
					//retrieve notifier - because object on k can vanish after the next command
					ISleepingObject *n = k->owner;
					//give ownership to k (see lock() - while (next != 0) )
					k->next = 0;
					//notify the thread if it is necessary
					n->wakeUp();
				}
			}
		}
	};



}


#endif /* LIGHTSPEED_MT_MICROLOCK_H_ */

