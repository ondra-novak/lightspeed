/*
 * timeoutException.h
 *
 *  Created on: 15.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_EXCEPTIONS_TIMEOUTEXCEPTION_H_
#define LIGHTSPEED_MT_EXCEPTIONS_TIMEOUTEXCEPTION_H_

#include "threadException.h"

namespace LightSpeed {

class TimeoutException: public ThreadException {
public:
	TimeoutException(const ProgramLocation &loc)
		:Exception(loc),ThreadException(loc) {}

	LIGHTSPEED_EXCEPTIONFINAL;

	static LIGHTSPEED_EXPORT const char *msgText;;
protected:

	void message(ExceptionMsg &msg) const {
		msg(msgText);
	}
};

}

#endif /* LIGHTSPEED_MT_EXCEPTIONS_TIMEOUTEXCEPTION_H_ */
