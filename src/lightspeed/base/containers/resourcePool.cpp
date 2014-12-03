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
	:resTimeout(restimeout),waitTimeout(waitTimeout),limit(limit)
{
	curLimit = limit;

}

AbstractResourcePool::~AbstractResourcePool() {
	AbstractResource *a= pool.pop();
	while (a) {
		try {
			delete a;
		} catch (...) {}
		a = pool.pop();
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
	do {
		AbstractResource *a= pool.pop();

		if (a == 0) {

			//allocate new resource if we are in limit
			if (lockDec(curLimit) >= 0) {
				try {
					AbstractResource *a = createResource();
					a->setTimeout(resTimeout);
					return a;
				} catch (...) {
					lockInc(curLimit);
					throw;
				}
			} else {
				lockInc(curLimit);
			}

			//create slot
			SyncPt::Slot s;
			//register slot
			wakePt.add(s);
			//check pool to cover adding resource to the pool since last check
			a = pool.pop();
			//still no resource
			if (a == 0) {
				//install timeout
				Timeout waitT(waitTimeout);
				//wait for resource
				wakePt.wait(s,waitT,SyncPt::interruptOnExit);
				//retrieve payload
				a = (AbstractResource *)s.payload;
				//if payload is null, it were timeout
				if (a == 0)
					//throw
					throw ResourceTimeoutException(THISLOCATION,getResourceName());
			} else {
				//we get resource - remove registration
				wakePt.remove(s);
				//now, it can happen that we will have two resources object
				//first taken by pop() second through payload - check
				AbstractResource *c = (AbstractResource *)s.payload;
				//test payload
				if (c) {
					//return extra resource back to the pool
					pool.push(a);
					a = c;
				}

			}
		}
		//check for resource expiration
		if (a->expired()) {
			//expired? destroy resource
			delete a;
			//increment the limit
			lockInc(curLimit);
			//get next resource - additionally destroy any expired
		}
		else {
			a->setTimeout(resTimeout);
			//finished - return resource
			return a;
		}
	}while (true);


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

class AbstractResourcePool::SetPayload {
public:
	SetPayload(void *p):p(p) {}
	void operator()(SyncPt::Slot *s) {
		s->payload = p;
	}
protected:
	void *p;
};

void AbstractResourcePool::release(AbstractResource *a) {
	if (curLimit < 0) {
		delete a;
	} else {
		if (!wakePt.notifyOne(SetPayload(a))) {
			pool.push(a);
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

