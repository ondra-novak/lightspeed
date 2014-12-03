#ifndef LIGHTSPEED_EXCEPTIONS_INVALIDNUMBERFORMAT_H_
#define LIGHTSPEED_EXCEPTIONS_INVALIDNUMBERFORMAT_H_

#include "errorMessageException.h"

namespace LightSpeed {
    
    
    
    class InvalidNumberFormatException: public Exception{
    public:
        LIGHTSPEED_EXCEPTIONFINAL;
        InvalidNumberFormatException(const ProgramLocation &loc,
                                     const ConstStrW &str)
        :Exception(loc),number(str) {}
        InvalidNumberFormatException(const ProgramLocation &loc,
                                     const ConstStrA &str)
        :Exception(loc),number(str) {}

        ConstStrW getErrorNumber() const {return number;}

        static LIGHTSPEED_EXPORT const char *msgText;

        virtual void message(ExceptionMsg &msg) const {
        	msg(msgText) << number;
        }

        virtual ~InvalidNumberFormatException() throw() {}

    protected:
        String number;



    };    
    
}


#endif /*INVALIDNUMBERFORMAT_H_*/
