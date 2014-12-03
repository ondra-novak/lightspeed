/*
 * truefalse.h
 *
 *  Created on: 7.9.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_TRUEFALSE_H_
#define _LIGHTSPEED_TRUEFALSE_H_


namespace LightSpeed {

    struct MTrue {
        int padding;
		static const bool value = true;

    };

    struct MFalse {
		static const bool value = false;
    };


    template<bool result>
    struct MBool: MFalse {};
    template<>
    struct MBool<true>: MTrue {};

}

#endif /* _LIGHTSPEED_TRUEFALSE_H_ */
