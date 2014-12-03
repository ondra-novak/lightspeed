/*
 * iexception.h
 *
 *  Created on: 15.9.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_BASE_IEXCEPTION_H_
#define _LIGHTSPEED_BASE_IEXCEPTION_H_

#include <exception>
#include "cloneable.h"
#include "export.h"


namespace LightSpeed {


    ///Minimal
    class IException: public std::exception {
    public:
        virtual const char *what() const throw () = 0;
        virtual void throwAgain() const = 0;
        virtual const IException *getReason() const = 0;
        virtual ~IException() throw () {}

    };


}

#endif /* _LIGHTSPEED_IEXCEPTION_H_ */
