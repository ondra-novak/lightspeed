/*
 * fiberException.h
 *
 *  Created on: 25.8.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_FIBEREXCEPTION_H_
#define LIGHTSPEED_MT_FIBEREXCEPTION_H_
#include "../../base/exceptions/systemException.h"

namespace LightSpeed {
class Fiber;

class FiberException: public virtual Exception {
	public:
		FiberException(const ProgramLocation &loc) {
			Exception::setLocation(loc);
		}
	};

	class NoCurrentFiberException: public FiberException {
	public:
		NoCurrentFiberException(const ProgramLocation &loc):FiberException(loc) {}
		LIGHTSPEED_EXCEPTIONFINAL;

		static LIGHTSPEED_EXPORT const char *msgText;;
	protected:
		void message(ExceptionMsg &msg) const {
			msg(msgText);
		}
	};

	class FiberNotRunningException: public FiberException {
	public:
		FiberNotRunningException(const ProgramLocation &loc, Fiber *fb)
				:FiberException(loc),fb(fb) {}
		LIGHTSPEED_EXCEPTIONFINAL;
		Fiber *getFiber() const {return fb;}
		static LIGHTSPEED_EXPORT const char *msgText;;
	protected:
		Fiber *fb;
		void message(ExceptionMsg &msg) const {
			msg(msgText) << setBase(16) << (natural)fb;
		}
	};

	class FiberErrorException: public FiberException, public ErrNoException {
	public:
		FiberErrorException(const ProgramLocation &loc, int errnr)
			:FiberException(loc),ErrNoException(loc,errnr) {}
		LIGHTSPEED_EXCEPTIONFINAL;
		static LIGHTSPEED_EXPORT const char *msgText;;
	protected:
		void message(ExceptionMsg &msg) const {
			msg(msgText);
			ErrNoException::message(msg);
		}
		virtual const char *getStaticDesc() const {
			return ErrNoException::getStaticDesc();
		}
	};




}  // namespace LightSpeed

#endif /* LIGHTSPEED_MT_FIBEREXCEPTION_H_ */
