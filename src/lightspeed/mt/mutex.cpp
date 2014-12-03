/*
 * mutex.cpp
 *
 *  Created on: 17.5.2013
 *      Author: ondra
 */

#include "mutex.h"

namespace LightSpeed {

	class Mutex::SetOwner {
	public:
		SetOwner(Mutex &mx):mx(mx) {}
		void operator()(SyncPt::Slot *s) {
			mx.owner = s->threadId;
			mx.recursion = 1;
		}

	protected:
		Mutex &mx;
	};



bool Mutex::lock( const Timeout &tm )
{
	//get my thread id
	atomic me = ThreadId::current().asAtomic();
	//try to put my thread id to owner - return prev owner
	atomic oth = lockCompareExchange(owner,0,me);
	//if there is prev owner
	if (oth != 0) {
		//and if it is me
		if (oth == me) {
			//increase recursion
			recursion++;
			//success
			return true;
		}
		//create slot
		SyncPt::Slot sl;
		//store new owner to the payload
		sl.threadId = me;
		//add slot to notification chain
		waitPt.add(sl);
		//test again for ownership - may be alread released
		if (lockCompareExchange(owner,0,me) != 0) {
			//in case of unsuccess - start waiting - we will be notified
			if (!waitPt.wait(sl,tm)) {
				//in case of timeout - remove self from notification
				waitPt.remove(sl);
				//recheck whether slot is signaled - if not, failure
				if (!sl) return false;
				//if does continue success.
			}
		} else {
			//remove self from chain, because we already have ownership
			waitPt.remove(sl);
		}
	}
	//in all cases, set recursion to 1
	recursion  = 1;

	return true;
}

void Mutex::lock()
{
	//lock with infinite timeout
	lock(naturalNull);
}

bool Mutex::tryLock()
{
	atomic me = ThreadId::current().asAtomic();
	if (lockCompareExchange(owner,0,me) != 0)
		return false;
	else
		return true;
}

void Mutex::unlock()
{
	//if recursion is above 1
	if (recursion > 1) {
		//decrease recursion
		recursion--;
		//exit
		return;
	}
	//notify one item in queue - it will set ownership
	if (!waitPt.notifyOne(SetOwner(*this))) {
		//if nothing released - write zero to ownership
		writeRelease(&owner,0);
		//check, whether, waitPt is still empty
		//if not and we are able to tryLock the lock
		if (!waitPt.empty() && tryLock()) {
			//unlock again to solve situation
			unlock();
		}
	}
}

bool Mutex::lockAsync( SyncPt::Slot &slot )
{
	//get my thread id
	atomic me = ThreadId::current().asAtomic();
	//try to put my thread id to owner - return prev owner
	atomic oth = lockCompareExchange(owner,0,me);
	//if there is prev owner
	if (oth != 0) {
		//and if it is me
		if (oth == me) {
			//increase recursion
			recursion++;
			//success - notify slot
			slot.notify(0);
			return true;
		}
		//set thread id - new owner
		slot.threadId = me;
		//register slot
		waitPt.add(slot);
		//test again for ownership - may be alread released
		if (lockCompareExchange(owner,0,me) != 0) {
			//if not, report that lock has not been locked and start
			//wait for slot
			return false;
		} else {
			//remove slot
			waitPt.remove(slot);
		}
	}
	//set recursion
	recursion = 1;
	//notify slot
	slot.notify(0);
	return true;
}

void Mutex::cancelAsync( SyncPt::Slot &sl )
{
	waitPt.remove(sl);
	atomic me = ThreadId::current().asAtomic();
	if (sl && me == readAcquire(&owner)) unlock();
}

bool Mutex::setOwner( atomic newOwner )
{
	atomic curOwner = ThreadId::current().asAtomic();
	return lockCompareExchange(owner,curOwner,newOwner) == curOwner;
}

}