
#ifndef LIGHTSPEED_ERRORMESSAGEEXCEPTION_H_
#define LIGHTSPEED_ERRORMESSAGEEXCEPTION_H_

#include "../containers/string.h"

namespace LightSpeed
{

    class ErrorMessageException: public virtual Exception {
    public:
        LIGHTSPEED_EXCEPTIONFINAL;

        ErrorMessageException(const ProgramLocation &loc,
                            const String &text)
                            : Exception(loc),textDesc(text) {}

        const String &getErrorMsg() const {return textDesc;}
    
        virtual ~ErrorMessageException() throw() {}

    protected:
        String textDesc;

        void message(ExceptionMsg &msg) const {
        	msg("%1") << textDesc;
        }

        virtual const char *getStaticDesc() const  {
            return "This kind exception is used in rare cases to "
                "report situations with very low probability, "
                "or to report general failures. ";              
        }
    };


} // namespace LightSpeed

#endif /*ERRORMESSAGEEXCEPTION_H_*/
