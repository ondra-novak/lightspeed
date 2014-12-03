/*
 * firstchancex.cpp
 *
 *  Created on: 28. 4. 2014
 *      Author: ondra
 */
#include "../exceptions/exception.h"
#include "../framework/iapp.h"

namespace LightSpeed {


struct FChExStatic {
	DWORD fchexp_lock;
	DWORD lastKnownException;

	FChExStatic() {
		fchexp_lock = TlsAlloc();
		lastKnownException = TlsAlloc();
	}

	~FChExStatic() {
		TlsFree(fchexp_lock);
		TlsFree(lastKnownException);
	}

	bool getLock() const {
		return TlsGetValue(fchexp_lock) !=0;
	}
	void setLock(bool lk) const {
		TlsSetValue(fchexp_lock,lk?&lk:0);
	}

	const Exception *getLastKnowException() const {
		return (const Exception *)TlsGetValue(lastKnownException);
	}

	void setLastKnowException(const Exception *e) const {
		TlsSetValue(lastKnownException,(LPVOID) e);
	}
};


static FChExStatic expInfo;

static void myOwnTerminate() {

	if (expInfo.getLastKnowException() != 0) {
		IApp *app = IApp::currentPtr();
		if (app) {
			try  {
				app->onException(*expInfo.getLastKnowException());
			} catch (...) {

			}
		}
	}
	abort();

}

void firstChanceException(const Exception *exception) {

	IApp *app = IApp::currentPtr();
	if (app) {
		if (!expInfo.getLock()) {
			expInfo.setLock(true);
			try {
				app->onFirstChanceException(exception);
			} catch (...) {

			}
			expInfo.setLock(false);
		}

		if (!std::uncaught_exception()) {
			expInfo.setLastKnowException(exception);
			std::set_terminate(&myOwnTerminate);
		}
}


}

void exceptionDestroyed(const Exception *exception) {
	if (expInfo.getLastKnowException() == exception) {
		expInfo.setLastKnowException(0);
	}
}
}
