#ifndef LIGHTSPEED_EXCEPTIONS_STDEXCEPTION_H_
#define LIGHTSPEED_EXCEPTIONS_STDEXCEPTION_H_

#include "exception.h"
#include "../containers/string.h"


namespace LightSpeed {
    
    ///Wraps all std exceptions to LightSpeed::Exception object 
    /**
    * This is the general way, how to handle standard exceptions thrown
    * from third part library or from STL itself and rethrow them as
    * LightSpeed::Exception to allow reason chains.
    * 
    * @note after standard exception is catched and wrapped to the StdException,
    * you are unable to access original exception object, because standard
    * exceptions cannot be cloned. Always handle all known exceptions regulary,
    * and only if unexpected standard exception is catched, you can wrap it
    * to the StdException object
    * 
    * @see NonStdException
    * 
    */
    class StdException: public Exception {
    public:
        LIGHTSPEED_EXCEPTIONFINAL;
        ///Constructs wrap on standard exception
        /** 
         * @param e reference to standard exception object. Function will
         *  extract exception description and exception type
         * @param loc location describes where this happened
         */
        StdException(const std::exception &e, const ProgramLocation &loc)
            :Exception(loc),what(getWhatForException(e)),type(typeid(e)) {
        	checkReason(e);
        }
        ///Constructs wrap on standard exception
        /** 
         * @param loc location describes where this happened
         * @param e reference to standard exception object. Function will
         *  extract exception description and exception type
         */
        StdException(const ProgramLocation &loc, const std::exception &e)
            :Exception(loc),what(getWhatForException(e)),type(typeid(e)) {
        	checkReason(e);
        }
        
        ///Retrieves "what" text from the exception
        const String &getWhat() const {return what;}
        ///Retrieves type information about exception
        const std::type_info &getType() const {return type;}
        virtual ~StdException() throw () {}

    protected:
        String what;
        const std::type_info &type;
        virtual void message(ExceptionMsg &msg) const {
        	msg("%1 (%2)") <<  what << type.name();
        }

        static String getWhatForException(const std::exception &e) {
        	const Exception *ee = dynamic_cast<const Exception *>(&e);
        	if (ee) {
        		return String("LightSpeed exception caught as StdException (see reason)");
        	} else {
        		return String(e.what());
        	}
        }

        void checkReason(const std::exception &e) {
        	const Exception *ee = dynamic_cast<const Exception *>(&e);
        	if (ee) setReason(*ee);
        }

    };   
    
    ///Wrapper around unknown exception
    /** Thus allows to wrap any unrecognized exception into
     * LightSpeed exception. Unfortunately,  result is
     * exception with no description. Use this wrapping class
     * only when your code catches unknown exception, that cannot be
     * rethrown to the upper level, but you need to record this
     * event as Lightspeed exception.
     */
    class UnknownException: public Exception {
    public:
        LIGHTSPEED_EXCEPTIONFINAL;

        UnknownException(const ProgramLocation &loc):Exception(loc) {}

        static LIGHTSPEED_EXPORT const char *msgText;;
    protected:
        virtual void message(ExceptionMsg &msg) const {
            msg(msgText);
        }
    };


}


#endif /*STDEXCEPTION_H_*/
