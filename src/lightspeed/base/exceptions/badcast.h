/*
 * badcast.h
 *
 *  Created on: 18.4.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_EXCEPTION_BADCAST_H_
#define LIGHTSPEED_EXCEPTION_BADCAST_H_

#include "exception.h"

namespace LightSpeed {

	class BadCastException: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		BadCastException(const ProgramLocation &loc,
				const std::type_info &source,
				const std::type_info &target)
			:Exception(loc),source(source),target(target) {}

		const std::type_info &getSource() const {return source;}
		const std::type_info &getTarget() const {return target;}

	protected:
		const std::type_info &source;
		const std::type_info &target;

		static LIGHTSPEED_EXPORT const char *msgText;;

		void message(ExceptionMsg &msg) const {
			msg(msgText) << source.name() << target.name();
		}
	};



}


#endif /* BADCAST_H_ */
