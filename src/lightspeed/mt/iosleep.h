/*
 * threadSleeper.h
 *
 *  Created on: 17.6.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_IOSLEEP_H_
#define LIGHTSPEED_MT_IOSLEEP_H_



#include "timeout.h"
#include "platform.h"
#if defined(LIGHTSPEED_PLATFORM_WINDOWS)
#include "windows/IOSleep.h"
#elif defined(LIGHTSPEED_PLATFORM_LINUX)
#include "linux/IOSleep.h"
#else
#error unimplemented feature
#endif

namespace LightSpeed {


	class IOSleep;

}



#endif /* LIGHTSPEED_MT_IOSLEEP_H_ */
