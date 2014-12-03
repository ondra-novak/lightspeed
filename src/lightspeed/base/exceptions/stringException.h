/*
 * stringException.h
 *
 *  Created on: 13.2.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_EXCEPTION_STRINGEXCEPTION_H_
#define LIGHTSPEED_EXCEPTION_STRINGEXCEPTION_H_

#pragma once

#include "exception.h"
#include "systemException.h"

namespace LightSpeed {

	class UnknownCodePageException: public ErrNoException {
	public:

		UnknownCodePageException(const ProgramLocation &loc, CodePageDef cp, int errnr)
			:ErrNoException(loc,errnr),cp(cp) {}

		const CodePageDef &getCodePage() const {return cp;}

		static LIGHTSPEED_EXPORT const char *msgText;;

	protected:
		CodePageDef cp;

		void message(ExceptionMsg &msg) const {
			msg(msgText) << cp;
			ErrNoException::message(msg);
		}

	};

	class InvalidCharacterException: public ErrNoException {
	public:

		InvalidCharacterException(const ProgramLocation &loc, char chr, int errnr)
			:ErrNoException(loc,errnr),chr(chr) {}

		const char &getCodePage() const {return chr;}

		static LIGHTSPEED_EXPORT const char *msgText;;

	protected:
		char chr;

		void message(ExceptionMsg &msg) const {
			msg(msgText) << (int)chr;
			ErrNoException::message(msg);
		}

	};

}  // namespace LightSpeed


#endif /* LIGHTSPEED_EXCEPTION_STRINGEXCEPTION_H_ */
