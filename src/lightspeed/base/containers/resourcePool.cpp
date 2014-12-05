/*
 * resourcePool.cpp
 *
 *  Created on: 13.5.2013
 *      Author: ondra
 */

#include "resourcePool.h"
#include "../framework/proginstance.h"
#include "../../mt/exceptions/timeoutException.h"

using LightSpeed::ProgInstance;


namespace LightSpeed {

using namespace LightSpeed;

AbstractResource::AbstractResource()
	:next(0),expiration(nil)
{
}

AbstractResource::~AbstractResource() {
}

bool AbstractResource::expired() const {
	return expiration.expired();
}

AbstractResourcePool::AbstractResourcePool(natural limit, natural restimeout, natural waitTimeout)
	:pool(0),resTimeout(restimeout),waitTimeout(waitTimeout),limit(limit)
{
	curLimit = limit;

}

AbstractResourcePool::~AbstractResourcePool() {
	while (pool) {
		AbstractResource *x = pool;
		pool = pool->next;
		try {
			delete x;
		} catch (...) {}
	}
}

class ResourceTimeoutException: public TimeoutException {
public:
	ResourceTimeoutException(const ProgramLocation &loc, const char *resourceName)
		:Exception(loc),TimeoutException(loc),resourceName(resourceName) {}

	LIGHTSPEED_EXCEPTIONFINAL;

	static const char *&msgText;

	const char *resourceName;
protected:

	void message(ExceptionMsg &msg) const {
		msg("Resource acquire timeout: %1") << resourceName;
	}
};



AbstractResource *AbstractResourcePool::acquire() {
	// lock SyncPt queue
	Synchronized<FastLock> _(SyncPt::queueLock);
	//reset out
	AbstractResource *out = 0;
	do {
		//if no items in the pool
		if (pool == 0) {
			//if still space for new resource
			if (readAcquire(&curLimit) > 0) {
				//unlock queue - don't need it anymore
				SyncReleased<FastLock> __(SyncPt::queueLock);
				//create new resource
				AbstractResource *a = createResource();
				//set resource timeout
				a->setTimeout(resTimeout);
				//decrease limit
				lockDec(curLimit);
				//this is new resource
				return a;
			}
			//install slot
			SyncPt::Slot s;
			//add slot to queue
			SyncPt::add_lk(s);
			{
				//unlock queue - we will wait
				SyncReleased<FastLock> __(SyncPt::queueLock);
				//install timeout
				Timeout waitT(waitTimeout);
				//wait for resource
				SyncPt::wait(s,waitT,SyncPt::interruptOnExit);
				//try to remove from queue, if successed, timeout
				if (SyncPt::remove(s)) {
					//report timeout
					throw ResourceTimeoutException(THISLOCATION,getResourceName());
				}
				//retrieve resource from the slot
				out = (AbstractResource *)s.payload;
			}

		} else {
			//in case that pool is not empty
			out = pool;
			//retrieve from pool and set pool's new head
			pool = pool->next;
		}

		//check resource expired
		if (out->expired()) {
			//if expired, unlock queue
			SyncReleased<FastLock> __(SyncPt::queueLock);
			//delete resource
			delete out;
			//increase limit
			lockInc(curLimit);
			//reset out
			out = 0;
		} else  {
			//set out's timeout
			out->setTimeout(resTimeout);
		}

		//repeat if we hase no out

	}while(out == 0);
	//unlock queue a return out
	return out;

}

void AbstractResourcePool::setLimit(natural newLimit) {
	if (newLimit > limit) {
		lockExchangeAdd(curLimit,newLimit - limit);
		newLimit = limit;
	} else if (newLimit < limit) {
		lockExchangeSub(curLimit,limit - curLimit);
		newLimit = limit;
	}
}

natural AbstractResourcePool::getLimit() const {
	return limit;
}

natural AbstractResourcePool::getCurLimit() const {
	return curLimit;
}

void AbstractResourcePool::release(AbstractResource *a) {
	//check limit, if it is negative, do not store resource
	if (readAcquire(&curLimit) < 0) {
		//delete resource
		delete a;
		//increase limit
		lockInc(curLimit);
	} else {
		//lock queue
		Synchronized<FastLock> _(SyncPt::queueLock);
		//pop slot
		SyncPt::Slot *s = SyncPt::popSlot_lk();
		//we poped slot?
		if (s == 0) {
			//no, store resource to pool
			a->next = pool;
			//set new head
			pool = a;
		} else {
			//give resource to slot
			s->payload = a;
			//notify it
			SyncPt::notifySlot_lk(s);
		}
	}
}

void AbstractResource::setTimeout(LightSpeed::Timeout expiration) {
	this->expiration = expiration;
}

AbstractResourcePtr::AbstractResourcePtr(AbstractResourcePool& pool)
	:pool(pool)
{
	ptr = pool.acquire();
}

AbstractResourcePtr::~AbstractResourcePtr() {
	if (!isShared()) pool.release(ptr);
}

} /* namespace jsonsrv */

