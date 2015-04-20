/*
 * canceledException.h
 *
 *  Created on: 13. 4. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_EXCEPTIONS_CANCELEDEXCEPTION_H_
#define LIGHTSPEED_BASE_EXCEPTIONS_CANCELEDEXCEPTION_H_
#include "exception.h"

namespace LightSpeed {

	class CanceledException: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		CanceledException(const ProgramLocation &loc):Exception(loc) {}


		static LIGHTSPEED_EXPORT const char *msgText;
	protected:

		virtual void message(ExceptionMsg &msg) const{
			msg(msgText);
		}

	};


}




#endif /* LIGHTSPEED_BASE_EXCEPTIONS_CANCELEDEXCEPTION_H_ */
