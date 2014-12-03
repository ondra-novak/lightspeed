/** @file
 * Contains declarations for most common exceptions thrown when
 *  system call failed (in connection with setting errno)
 *
 * $Id: systemException.h 3640 2013-08-23 14:58:05Z ondrej.novak $
 */

#include "../exceptions/exception.h"

#ifndef LIGHTSPEED_BASE_EXCEPTION_SYSTEMEXCEPTION_
#define LIGHTSPEED_BASE_EXCEPTION_SYSTEMEXCEPTION_

#include "../containers/string.h"
#include <errno.h>
//#include <string>

namespace LightSpeed
{

    
class  SystemException: public virtual Exception {
public:
    SystemException(const ProgramLocation &loc):Exception(loc) {
    	Exception::setLocation(loc);
    }
};
    
    
///Exception, that contains errno of operation
/**
 * When a system operation failed, errno contains error code of this fail.
 * You can throw errno in this exception. If exception is catched as
 * std::exception, description will contain error code and error message
 */
class  ErrNoException: public SystemException{

public:
    
	LIGHTSPEED_EXCEPTIONFINAL;
    ///Constructor
    /**
     * @param loc location in the program
     * @param error_nr errno code
     */
    ErrNoException(const ProgramLocation &loc, int error_nr)
        :Exception(loc)
        ,SystemException(loc)
        ,error_nr(error_nr) {}


    virtual const char *getStaticDesc() const {
        return "Exception contains stored errno. It is thrown when system call "
                "failed and error cannot be handled by different way";
    };


    ///Retrieves errno carried by this application
    int getErrNo() const {
        return error_nr;
    }
    virtual ~ErrNoException() throw() {}

    static LIGHTSPEED_EXPORT const char *msgText;;

protected:
    int error_nr;

    static void getSystemErrorMessage(ExceptionMsg &msg, int error_nr);

    virtual void message(ExceptionMsg &msg) const {
    	msg(msgText) << error_nr;
        getSystemErrorMessage(msg,error_nr);
    }



};


///Extension of ErrNoException, it can carry text description.
/** Description should describe operation, which failed. For example
 * "mmap has failed" or "unable to open file"
 *
 * This exception is useful if you want to throw unexcepted errors
 * that will not probably catched for handling, only for reporting. It
 * is always better to create new exception class for the specific
 * situation instead using this exception.
 */
class ErrNoWithDescException: public ErrNoException {
public:

    LIGHTSPEED_EXCEPTIONFINAL;

    ///Constructor
    /**
     * @param loc program location (USE THISLOCATION)
     * @param error_nr errno
     * @param desc text description
     */
    
    
    ErrNoWithDescException(const ProgramLocation &loc,
                             int error_nr,
                             const String &desc)
        :Exception(loc)
        ,ErrNoException(loc,error_nr), desc(desc) {}

    
    virtual ~ErrNoWithDescException() throw() {}

    ///Retrieves the description
    const String &getDesc() const {return desc;}

    String desc;

    virtual void message(ExceptionMsg &msg) const {

    	msg("%1 - ") << desc;
        ErrNoException::message(msg);
    }
};

} // namespace LightSpeed

#endif /*LIGHTSPEED_BASE_EXCEPTION_SYSTEMEXCEPTION_*/
