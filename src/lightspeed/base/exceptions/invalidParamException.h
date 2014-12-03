/*
 * invalidParamException.h
 *
 *  Created on: 27.2.2010
 *      Author: ondra
 */

#include "exception.h"

#pragma once
#include "../containers/string.h"

namespace LightSpeed {

	class InvalidParamException: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		InvalidParamException(const ProgramLocation &loc, natural paramPos,
			const String &desc)
				:Exception(loc),paramPos(paramPos),desc(desc) {}

		const String getDesc() const;
		natural getParamPos() const;

		virtual ~InvalidParamException() throw() {}

	protected:
		natural paramPos;
		String desc;

		static LIGHTSPEED_EXPORT const char *msgText;;

		void message(ExceptionMsg &msg) const {
			msg(msgText) << paramPos << desc;
		}
	};


}
