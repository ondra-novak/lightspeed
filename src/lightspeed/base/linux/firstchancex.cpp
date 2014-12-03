/*
 * firstchancex.cpp
 *
 *  Created on: 28. 4. 2014
 *      Author: ondra
 */
#include "../exceptions/exception.h"
#include "../framework/iapp.h"

namespace LightSpeed {


static __thread bool fchexp_lock=false;
static __thread const Exception *lastKnownException;

static void myOwnTerminate() {

	if (lastKnownException != 0) {
		IApp *app = IApp::currentPtr();
		if (app) {
			try  {
				app->onException(*lastKnownException);
			} catch (...) {

			}
		}
	}
	abort();

}

void firstChanceException(const Exception *exception) {

	IApp *app = IApp::currentPtr();
	if (app) {
		if (!fchexp_lock) {
			fchexp_lock = true;
			try {
				app->onFirstChanceException(exception);
			} catch (...) {

			}
			fchexp_lock = false;
		}

		if (!std::uncaught_exception()) {
			lastKnownException = exception;
			std::set_terminate(&myOwnTerminate);
		}
}


}

void exceptionDestroyed(const Exception *exception) {
	if (lastKnownException == exception) {
		lastKnownException = 0;
	}
}
}
