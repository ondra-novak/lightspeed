/*
 * isConvertible.h
 *
 *  Created on: 7.9.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_ISCONVERTIBLE_H_
#define _LIGHTSPEED_ISCONVERTIBLE_H_

#include "truefalse.h"

namespace LightSpeed {


#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4244 )
#endif
    template<class From, class To>
    struct MIsConvertible {

        static MTrue _check(To x);
        static MFalse _check(...);
        static From x;

        static const bool value = sizeof(_check(x)) == sizeof(MTrue);

        typedef MBool<value> MValue;
    };


#ifdef _MSC_VER
#pragma warning( pop )
#endif

    template<class From>
    struct MIsConvertible<From,void> {

        static const bool value = false;
        typedef MFalse MValue;
    };

    template<class From>
    struct MIsConvertible<void,From> {

        static const bool value = false;
        typedef MFalse MValue;
    };


    template<>
    struct MIsConvertible<void,void> {

        static const bool value = false;
        typedef MFalse MValue;
    };

}

#endif /* _LIGHTSPEED_ISCONVERTIBLE_H_ */
