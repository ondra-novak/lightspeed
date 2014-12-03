/*
 * isAbstract.h
 *
 *  Created on: 12.9.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_META_ISABSTRACT_H_
#define _LIGHTSPEED_META_ISABSTRACT_H_

#include "../types.h"
#include "truefalse.h"

namespace LightSpeed {


    namespace _intr {

        template<class T, natural sz>
        struct IsAbstract_Imp2 {

            template<typename U>
            static MFalse _check(U (*x)[1]);
            template<typename U>
            static MTrue _check(...);

            static const bool value = sizeof(_check<T>(0)) == sizeof(MTrue);
        };

        template<typename T>
        struct IncompleteTypeIsAbstract;

        template<typename T>
        struct IsAbstract_Imp2<T,0> {
            IncompleteTypeIsAbstract<T> value;
        };
    }


    template<class T>
    struct IsAbstract: public _intr::IsAbstract_Imp2<T,sizeof(T)> {};


}

#endif /* _LIGHTSPEED_ISABSTRACT_H_ */
