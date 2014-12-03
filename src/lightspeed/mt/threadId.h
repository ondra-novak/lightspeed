#ifndef LIGHTSPEED_MT_THREADID_H_
#define LIGHTSPEED_MT_THREADID_H_

#include "platform.h"

#ifdef LIGHTSPEED_PLATFORM_WINDOWS
#include "windows/threadId.h"
#endif

#ifdef LIGHTSPEED_PLATFORM_LINUX
#include "linux/threadId.h"
#endif

namespace LightSpeed
{

    class ThreadId;

} // namespace LightSpeed

#endif /*THREADID_H_*/
