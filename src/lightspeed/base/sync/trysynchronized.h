/*
 * trysynchronized.h
 *
 *  Created on: 18.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_SYNC_TRYSYNCHRONIZED_H_
#define LIGHTSPEED_BASE_SYNC_TRYSYNCHRONIZED_H_

#include "synchronize.h"
#include "../exceptions/exception.h"

namespace LightSpeed {


///Exception is thrown, when TrySynchronized object cannot obtain lock
class SynchronizedException: public Exception {
public:
	SynchronizedException(const ProgramLocation &loc)
		:Exception(loc) {}
	LIGHTSPEED_EXCEPTIONFINAL;

	static LIGHTSPEED_EXPORT const char *msgText;;

protected:
	void message(ExceptionMsg &msg) const {
		msg(msgText);
	}
};

///Exception is thrown, when TrySynchronized object cannot obtain lock.
/**
 * @tparam T type of lock. Helps to find location of exception if not handled
 */
template<class T>
class SynchronizedExceptionT: public SynchronizedException {
public:
	SynchronizedExceptionT(const ProgramLocation &loc)
		:SynchronizedException(loc) {}
	LIGHTSPEED_EXCEPTIONFINAL;
protected:
	void message(ExceptionMsg &msg) const {
		SynchronizedException::message(msg);
		msg(": %1") <<typeid(T).name();
	}
};

///Acquires the lock if available or throws exception
/** Class is useful to implement tryLock-unlock sections */
template<class T>
class TrySynchronized: public Synchronized<T> {
public:

	TrySynchronized(T &lk):Synchronized<T>(lk) {
		if (lk.tryLock() == false)
			throw SynchronizedExceptionT<T>(THISLOCATION);
	}
};




}  // namespace LightSpeed

#endif /* LIGHTSPEED_BASE_SYNC_TRYSYNCHRONIZED_H_ */
