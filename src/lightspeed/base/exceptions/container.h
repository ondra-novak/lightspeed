#pragma once
#include "exception.h"

namespace LightSpeed {


	class ContainerIsEmptyException : public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		ContainerIsEmptyException(const ProgramLocation &loc) :Exception(loc) {}

		static LIGHTSPEED_EXPORT const char *msgText;

		virtual void message(ExceptionMsg &msg) const{
			msg(msgText);
		}
	};

}