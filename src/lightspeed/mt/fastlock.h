/*
 * fastlock.h
 *
 *  Created on: 26.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_FASTLOCK_H_
#define LIGHTSPEED_MT_FASTLOCK_H_

#include "fastlock.h"
#include "atomic.h"
#include "threadMinimal.h"
#include "microlock.h"



namespace LightSpeed {


///FastLock can be used to implement very fast locking mechanism
/** FastLock is implemented in user-space using inter-locked operations
 * and simple queue of threads. It doesn't support asynchronous locking and
 * timeouted waiting. Once thread starts waiting, it cannot be interrupted
 * until object is unlocked.
 *
 * FastLock takes only 8 (16) bytes and doesn't take any system resources
 * when it is idle.
 *
 * FastLock is designed to lock piece of code where state of data
 * can be inconsistent for a while. Because it is implemented in user-space
 * it doesn't enter to the kernel until it is really necessary (in conflicts)
 *
 * FastLock uses Thread's sleep/wakeUp feature and requires Thread object
 * associated with the current thread, otherwise it fails.
 */
class FastLock {

protected:
	class Slot {
	public:
		Slot(ISleepingObject *n):next(0),notify(n) {}

		Slot *next;
		ISleepingObject *notify;
	};

public:

	FastLock():queue(0),owner(0) {}

	///Lock the object
	/**
	 * When object is unlocked, function immediately return setting
	 * current thread as new owner
	 *
	 * When object is locked, function will wait until other thread
	 * unlocks the object.
	 *
	 * You cannot set timeout due simplicity of the object
	 */
	void lock() {
		//fast way how to lock unlocked lock
		//it doesn't need to retrieve current thread instance
		//when queue is empty - no owner - use tryLock
		//(acqure barrier here to get actual value
		if (readAcquirePtr(&queue) != 0  || !tryLock()) {
			//otherwise - use standard way
			lockSlow();
		}

	}


	///Unlocks the object
	/**Because lock is not bound to thread-id, it is valid operation
	 * to unlock object in other thread. Ensure, that you know what you
	 * doing. Also ensure, that you not trying to unlock already unlocked
	 * object.
	 */
	void unlock() {
		//read owner
		Slot *o = owner;
		//set owner to 0 - it should not contain invalid value after unlock
		owner = 0;
		//try to unlock as there is no threads in queue (release barier)
		Slot *topQ = lockCompareExchangePtr<Slot>(&queue,o,0);
		//in case, that other thread waiting, topQ will be different than owner
		if (topQ != o) {
			//p = queue iterator
			Slot *p = topQ;
			//find tail of the queue - previous object will be new owner
			while (p->next != o) p = p->next;
			//store target thread, setting new owner can make Slot disappear
			ISleepingObject *ntf = p->notify;
			//write owner to notify next thread - target thread has ownership now!
			owner = p;
			//notify new owner - it may sleep (release barier here)
			ntf->wakeUp();
		}

	}

	///Tries to lock object without waiting
	/** In case, that lock is not successful, function fails returning
	 * false.
	 * @retval true successfully locked
	 * @retval false lock is already locked
	 */
	bool tryLock() {
		//register empty slot - for stack address only
		Slot s(0);
		//try to put slot to the top of queue if queue is empty
		Slot *tmp = lockCompareExchangePtr<Slot>(&queue,0,&s);
		//queue were empty
		if (tmp == 0) {
			//set owner
			owner = &s;
			//lock success
			return true;
		} else {
			//queue was not empty
			//failure
			return false;
		}

	}


protected:

	///top of queue
	Slot * volatile queue;
	///address of owner and also end of queue (end of queue is not NULL!)
	Slot * volatile owner;

	///registers slot to the queue
	/**
	 * @param slot slot to register
	 * @retval true slot has been registered - do not destroy slot until
	 *   it is notified. Also do not call unlock() before notification
	 * @retval false queue has been empty, registration is not needed,
	 * 	 ownership is granted, you can destroy slot without harm. If you
	 * 	 no longer need to own lock, don't forget to call unlock()
	 */
	bool addToQueue(Slot *slot) {
		//get top of queue
		Slot *tmp = queue,*tmp2;
		do {
			tmp2 = tmp;
			//set next pointer to current top
			slot->next = tmp2;
			//trade top queue with new slot if it has not changed
			tmp = lockCompareExchangePtr(&queue,tmp2,slot);
			//repeat trade in case of failure
		} while (tmp != tmp2);

		return tmp != 0;
	}

	void lockSlow()
	{
		//setup waiting slot
		Slot s(getCurThreadSleepingObj());
		//try to add to queue
		if (addToQueue(&s)) {
			//if added, repeatedly test notifier until it is notified
			//cycle is required to catch and ignore unwanted notifications
			//from other events - full barier in pauseThread()
			while (owner != &s) threadHalt();
		} else {
			//mark down owner
			owner = &s;
			//object is locked, temporary variables are no longer needed
		}
	}

};

///Recursive fastLock
class FastLockR: private FastLock {
public:
	FastLockR():ownerThread(0),recursiveCount(0) {}

	///Lock FastLock 
	/** Function also takes care on recursive locking which causes deadlock on standard FastLock.
	   If lock is already owned by the current tread, function doesn't block. Instead it counts recursions */
	void lock() {
		//fast way how to lock unlocked lock
		//it doesn't need to retrieve current thread instance
		//when queue is empty - no owner - use tryLock
		//(acqure barrier here to get actual value
		if (!tryLock()) {
			//otherwise - use standard way
			lockSlow();
			
			ownerThread = getCurThreadId();
			//set recursive count to 1
			recursiveCount=1;
		}

	}

	///function is used when recursive locking is issued
	/**
	 * Lock that allows recursive locking should define this function. Non-recursive lock will fail to compile
	 */
	void lockR() {
		lock();
	}

	///Try to lock the fastlock
	/** Function also takes care on recursive locking which causes deadlock on standard FastLock.
	   If lock is already owned by the current tread, function doesn't return false. Instead it counts recursions and returns true
	   
	   @retval true locked and owned
	   @retval false not locked and not owned
	   */
	
	bool tryLock() {
		atomicValue cid = getCurThreadId();
		//try lock 
		if (FastLock::tryLock()) {
			//if success, set setup recursive count
			recursiveCount=1;
			ownerThread = cid;
			//return success
			return true;
		}
		
		//check whether we are already owning the fast lock
		if (ownerThread == cid) {
			//YES, increase recursive count
			recursiveCount++;

			return true;
		}
		//return success
		return false;
	}

	///Unlock the fastlock
	/**
	  Function also tajes care on recursive locking. Count of unlocks must match to the count of locks (including
	  successed tryLocks). After these count matches, lock is released
	  */
	void unlock() {

		if (ownerThread == getCurThreadId() && recursiveCount > 0) {

			if (--recursiveCount == 0) {

				ownerThread = 0;

				FastLock::unlock();

			
			}

		}

	}

	///Unlocks the lock saving the recursion count
	/** This function unlocks the lock regardless on how many times the lock has been locked. Function
	saves this number to be used later with lockRestoreRecursion()

	@retval count of recursions
	*/
	 
	natural unlockSaveRecusion() {
		natural ret = recursiveCount;
		recursiveCount = 0;
		FastLock::unlock();
		return ret;
	}

	///Locks the lock and restores recursion count
	/**
	 @param count saved by unlockSaveRecursion()
	 @param tryLock set true to use tryLock(), otherwise lock() is used
	 @retval true lock successful
	 @retval false lock failed. When tryLock=true, function is unable to perform locking, because
	   the lock is owned (regardless on who is owner). When tryLock=false the lock is already owned by this
	   thread, so function cannot restore recursion counter. 
	  */
	 
	bool lockRestoreRecursion(natural count, bool tryLock) {
		//if tryLock enabled
		if (tryLock) {
			//call original tryLock - it returns false, when lock is owned
			if (!FastLock::tryLock()) return false;
		} else {
			//try to lock
			lock();
			//test whether recursive count is 1
			if (recursiveCount != 1) {
				//no, lock has been already locked before, we unable to restore recursion count
				unlock();
				//return false
				return false;
			}
		}
		//restore recursion count
		recursiveCount = count;
		//success
		return true;
	}

protected:
	atomic ownerThread;
	atomic recursiveCount;


};




}

#endif /* LIGHTSPEED_MT_FASTLOCK_H_ */
