/*
 * waitException.h
 *
 *  Created on: 6.9.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_EXCEPTIONS_WAITEXCEPTION_H_
#define LIGHTSPEED_EXCEPTIONS_WAITEXCEPTION_H_

#pragma once

#include "systemException.h"

namespace LightSpeed {


class WaitingException: public ErrNoException {
public:
	LIGHTSPEED_EXCEPTIONFINAL;
	WaitingException(const ProgramLocation &loc, int errnr): ErrNoException(loc,errnr) {}

	static LIGHTSPEED_EXPORT const char *msgText;;
protected:
	void message(ExceptionMsg &msg) const {
		msg(msgText);
		ErrNoException::message(msg);
	}
};


}

#endif /* WAITEXCEPTION_H_ */
