/*
 * fastrwlock.h
 *
 *  Created on: 14.10.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_FASTRWLOCK_H_
#define LIGHTSPEED_MT_FASTRWLOCK_H_
#include "sleepingobject.h"

namespace LightSpeed {


///Very simple and fast Read/Write lock
/** while lockRead is issued, lock is requested to acquire resource for reading.
 * Reading is shared mode, there can be more threads, that are reading. But
 * once lock is locked for reading, any other lock for writing are blocked.
 *
 * Writing mode works similar as standard FastLock. Only one owner can own
 * lock locked for writing. This also block any attempts to lock for reading
 *
 */
class FastRWLock {
	static const atomic exclusiveFlag = atomic(1) << (sizeof(atomic) * 8 - 3);
	static const atomic counterMask = exclusiveFlag - 1;

public:



	FastRWLock():rcnt(0),curWaiting(0) {}

	///Lock for reading
	void lockRead() {
		//acquire internal lock - this blocks, when owner is keeping lock for writing
		lk.lock();
		//increment count of readers - this blocks any try to acquire lock for writing
		lockInc(rcnt);
		//unlock internal lock - other readers can continue
		lk.unlock();
	}

	void unlockRead() {
		//this can be called in locked mode (or not)
		//as first operation, decrease read counter
		//if it were last reader, we can notify waiting thread requesting
		//write lock... if there is any
		if ((lockDec(rcnt) & counterMask)== 0 && curWaiting != 0) {
			curWaiting->wakeUp();
		}
	}

	void lockWrite() {
		//acquired internal log - will be kept until lockWrite is unlocked
		lk.lock();
		//set exclusive flag, so other threads will see that lock is exclusive locked
		lockExchangeAdd(rcnt,exclusiveFlag);
		//repeat everytime thread is waken up
		//first increase count of readers - to know, whether last already leaved locked area
		while (lockInc(rcnt) > (1 + exclusiveFlag)) {
			//register self to waiting pointer - it will not be notified yet
			curWaiting = FastLock::initSlot();
			//decrease count of reades simulating last reader leaved section
			if (lockDec(rcnt) > exclusiveFlag) {
				//if this is not true - still readers, wait for release last one
				FastLock::pauseThread();
			}
		}
		//remove registration
		curWaiting = 0;
		//release final reader counter
		lockDec(rcnt);
	}

	void unlockWrite() {
		//remove exclusive flag
		lockExchangeSub(rcnt,exclusiveFlag);
		//unlock is simple - unlock internal lock
		lk.unlock();
	}

	///try lock for reading
	bool tryLockRead() {
		//try lock internal lock - note this can fail even if there are readers
		while (lk.tryLock() == false) {
			//so if it failed, test exclusive flag and give up when it is set
			if (readAcquire(&rcnt) & exclusiveFlag) return false;
			//otherwise, try it again.
		}
		//increase count of readers
		lockInc(rcnt);
		//unlock the lock
		lk.unlock();
		return true;
	}

	bool tryLockWrite() {
		//try to lock main lock
		if (lk.tryLock()) {
			//if successed, this is not victory
			//try lock should not block on waiting on release by readers
			//try to acquire one reader
			if (lockInc(rcnt) > 1) {
				//if there is still readers, bad - release reader
				if (lockDec(rcnt) > 0) {
					//and there should be zero, otherwise, it we cannot wait for readers
					//unlock and give up
					lk.unlock();
					return false;
				}
			}
			//and at this point, the lock is exclusive locked
			//set appropriate flag
			lockExchangeAdd(rcnt,exclusiveFlag);
			//and return success
			return true;
		} else {
			//we were unable to lock the main lock, fail.
			return false;
		}
	}


protected:

	FastLock lk;
	atomic rcnt;
	ISleepingObject *curWaiting;



};

}


#endif /* LIGHTSPEED_MT_FASTRWLOCK_H_ */
