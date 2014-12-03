/*
 * isDynamic.h
 *
 *  Created on: 5.10.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_META_ISDYNAMIC_H_
#define _LIGHTSPEED_META_ISDYNAMIC_H_

#include "primitiveType.h"
#include "metaIf.h"
#include "truefalse.h"

namespace LightSpeed {


    namespace _intr {

        template<typename T,bool c>
        struct IsDynamicImpl;

        template<typename T>
        struct IsDynamicImpl<T,true>: MTrue {};

        template<typename T>
        class IsDynamicImpl2: public T {
        public:
            virtual ~IsDynamicImpl2() {}
        };

        template<typename T>
        struct IsDynamicImpl<T,false>:
            MIf<(sizeof(T) < sizeof(IsDynamicImpl2<T>)), MFalse, MTrue> {};



        template<typename T, bool c>
        struct ObjectAddress;

        template<typename T>
        struct ObjectAddress<T,true> {
            static void *get(T *obj) {return dynamic_cast<void *>(obj);}
            static const void *get(const T *obj) {return dynamic_cast<const void *>(obj);}
            static natural getOffset(const T *obj) {
                return reinterpret_cast<const byte *>(get(obj)) - reinterpret_cast<const byte *>(obj);
            }
        };

        template<typename T>
        struct ObjectAddress<T,false> {
            static void *get(T *obj) {return reinterpret_cast<void *>(obj);}
            static const void *get(const T *obj) {return reinterpret_cast<const void *>(obj);}
            static natural getOffset(const T *) {return 0;}
        };


    }

    template<typename T>
    struct IsDynamic: public _intr::IsDynamicImpl<T,IsPrimitiveType<T>::value > {};


    ///Helps to receive object's base address
    /**
     * Object's base address is address, where object begins. Pointers
     * can refer object into middle of its body, and if we need to
     * receive base address, we have to move pointer to about some bytes up.
     * This is provided by dynamic_cast special version. Because you
     * cannot use dynamic cast to the primitive type or type with no
     * virtual table, this template allows you to retrieve base address
     * of primitive type, which will always equal to used pointer.
     *
     * Another function getOffset() allows you to receive count of bytes
     * to shift from pointer address to base address
     */
    template<typename T>
    struct ObjectAddress: public _intr::ObjectAddress<T,IsDynamic<T>::value > {};

}

#endif /* _LIGHTSPEED_ISDYNAMIC_H_ */
