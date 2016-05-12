#ifndef LIGHTSPEED_EXCEPTIONS_IOEXCEPTION_H_
#define LIGHTSPEED_EXCEPTIONS_IOEXCEPTION_H_
#include "./exception.h"



namespace LightSpeed
{

 
    ///helper exception class
    /** it defines group of exceptions triggered due IO operation failed
     * You can catch this exception
     */
    class IOException: public virtual Exception {
    public:
           
        IOException(const ProgramLocation &loc):Exception(loc) {}
        IOException():Exception(THISLOCATION) {}
        ~IOException() throw() {}
        virtual const char *getStaticDesc() const {
            return "This base exception describes all input output problems";
        }
    };
 
}
#endif /*IOEXCEPTION_H_*/
