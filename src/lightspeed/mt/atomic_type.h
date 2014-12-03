/*
 * atomic_type.h
 *
 *  Created on: 14.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_ATOMIC_TYPE_H_
#define LIGHTSPEED_MT_ATOMIC_TYPE_H_


#include "platform.h"

#ifdef LIGHTSPEED_PLATFORM_WINDOWS
#include "windows/atomic_type.h"
#endif

#ifdef LIGHTSPEED_PLATFORM_LINUX
#include "linux/atomic_type.h"
#endif


#endif /* LIGHTSPEED_MT_ATOMIC_TYPE_H_ */
