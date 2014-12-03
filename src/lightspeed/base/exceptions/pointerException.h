#include "exception.h"

#ifndef LIGHTSPEED_EXCEPTION_POINTEREXCEPTION_H_
#define LIGHTSPEED_EXCEPTION_POINTEREXCEPTION_H_

#include "errorMessageException.h"

namespace LightSpeed {


    class PointerException: public virtual Exception {
    public:
        PointerException(const ProgramLocation &loc):Exception(loc) {}
    };

    class NullPointerException: public PointerException {
    public:
        LIGHTSPEED_EXCEPTIONFINAL;

        NullPointerException(const ProgramLocation &loc)
            :Exception(loc),PointerException(loc) {
 
        }

    protected:
        virtual void message(ExceptionMsg &msg) const {
        	msg(msgText);
        }

        static LIGHTSPEED_EXPORT const char *msgText;;

        const char *getStaticDesc() const {
            return "Try to reference object with NULL or nil pointer";
        }
    };

    class InvalidPointerException: public PointerException {
    public:
        LIGHTSPEED_EXCEPTIONFINAL;

        InvalidPointerException(const ProgramLocation &loc, const void *ptrValue)
            :Exception(loc),PointerException(loc),ptrValue(ptrValue) {}

        const void *getPtr() const {return ptrValue;}

    protected:

        const void *ptrValue;

        virtual void message(ExceptionMsg &msg) const {
        	msg(msgText) << setBase(16) << (natural)ptrValue;
        }

        const char *getStaticDesc() const {
            return "Reference to an object is invalid (is not null)";
        }

        static LIGHTSPEED_EXPORT const char *msgText;;

    };


	
	class NullPointerExceptionDesc: public NullPointerException,
									public ErrorMessageException {
	public:
		NullPointerExceptionDesc(const ProgramLocation &loc, ConstStrW desc)
			:Exception(loc),NullPointerException(loc),ErrorMessageException(loc,desc) {}
		
		typedef NullPointerExceptionDesc ICloneableBase;
		LIGHTSPEED_EXCEPTIONFINAL;

	protected:
        virtual void message(ExceptionMsg &msg) const {
			NullPointerException::message(msg);
			msg(": ");
			ErrorMessageException::message(msg);
		}
		virtual const char *getStaticDesc() const {
			return NullPointerException::getStaticDesc();
		}

	};
	

} // namespace LightSpeed

#endif /*POINTEREXCEPTION_H_*/
