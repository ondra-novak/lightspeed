/*
 * semaphore.h
 *
 *  Created on: 21.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_SEMAPHORE_H_
#define LIGHTSPEED_MT_SEMAPHORE_H_

#include "syncPt.h"

namespace LightSpeed {


	///semaphore implemented using SyncPt
	class Semaphore: private SyncPt {
	public:

		typedef SyncPt::Slot Slot;
		///Initializes semaphore
		/**
		 * @param count count of locks before semaphore starts blocking
		 */
		Semaphore(atomicValue count):counter(count) {}

		///simple lock, infinite timeout
		void lock() {
			Slot slot;
			lockAsync(slot);
			SyncPt::wait(slot,nil);
		}

		///lock semaphore, specify condition to stop waiting
		bool lock(SyncPt::WaitInterruptMode md) {
			Slot slot;
			lockAsync(slot);
			if (!SyncPt::wait(slot,nil,md)) {
				return false;
			} else {
				return true;
			}
		}

		///lock semaphore and specify timeout and condition
		bool lock(const Timeout &tm, SyncPt::WaitInterruptMode md) {
			Slot slot;
			lockAsync(slot);
			if (!SyncPt::wait(slot,tm,md)) {
				return false;
			} else {
				return true;
			}
		}

		///lock semaphore and specify timeout
		bool lock(const Timeout &tm) {
			Slot slot;
			lockAsync(slot);
			if (!SyncPt::wait(slot,tm)) {
				return false;
			} else {
				return true;
			}
		}

		///call to unlock semaphore
		void unlock() {
			Synchronized<FastLock> _(queueLock);

			SyncPt::Slot *s = popSlot_lk();
			if (s == 0) {
				counter++;
			} else {
				notifySlot_lk(s);
			}
		}
		///test, whether semaphore can by locked
		bool tryLock() {
			Synchronized<FastLock> _(queueLock);
			if (counter > 0) {
				counter--;
				return true;
			} else {
				return false;
			}
		}

		void lockAsync(Slot &slot) {
			Synchronized<FastLock> _(queueLock);
			if (counter > 0) {
				counter--;
				notifySlot_lk(&slot);
			} else {
				add_lk(slot);
			}
		}
		void cancelLockAsync(Slot &slot) {
			//remove slot from the syncpt
			if (!SyncPt::remove(slot)) {
				//if slot is signaled, unlock semaphore
				if (slot) unlock();
			}
		}

		///Initialize semaphore using another semaphore
		/**
		 * @param s source semaphore - constructor takes only actual count of available locks
		 */
		Semaphore(const Semaphore &s):SyncPt(),counter(s.counter) {}

		///Adjust available locks for semaphore using another semaphore
		Semaphore &operator=(const Semaphore &s) {counter = s.counter;return *this;}

	protected:

		void cancelSlot(Slot &sl) {
			cancelLockAsync(sl);
		}


		natural counter;
	};

}




#endif /* LIGHTSPEED_MT_SEMAPHORE_H_ */
