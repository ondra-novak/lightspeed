/*
 * atomic_type.h
 *
 *  Created on: 14.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_WINDOWS_ATOMIC_TYPE_H_
#define LIGHTSPEED_MT_WINDOWS_ATOMIC_TYPE_H_

#include "../../base/types.h"

namespace LightSpeed {

#ifdef _WIN64

	///Const that has same type as atomic, but without volatile prefix
	typedef LONGLONG atomicValue;
	///type with enforced volatile prefix should be used to store atomic values
	typedef volatile atomicValue atomic;
#else
	///Const that has same type as atomic, but without volatile prefix
	typedef LONG atomicValue;
	///type with enforced volatile prefix should be used to store atomic values
	typedef volatile atomicValue atomic;
#endif
}


#endif /* LIGHTSPEED_MT_WINDOWS_ATOMIC_TYPE_H_ */
