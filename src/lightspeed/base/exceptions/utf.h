#ifndef LIGHTSPEED_EXCEPTIONS_UTF_H_
#define LIGHTSPEED_EXCEPTIONS_UTF_H_

#pragma once

#include "exception.h"


namespace LightSpeed
{

    class InvalidUTF8Character: public Exception {
    public:
        LIGHTSPEED_EXCEPTIONFINAL;
        
        InvalidUTF8Character(const ProgramLocation &loc, byte chr)
            :Exception(loc),chr(chr) {}
        byte getChar() const {return chr;}

        static LIGHTSPEED_EXPORT const char *msgText;;
    protected:
        byte chr;
        
        virtual void message(ExceptionMsg &msg) const {
        	msg(msgText) << setBase(16) << (natural)chr;
        }
    };

} // namespace LightSpeed

#endif /*UTF_H_*/
