#ifndef LIGHTSPEED_MT_PLATFORM
#define LIGHTSPEED_MT_PLATFORM
#pragma once

#if defined(_WIN64)

#define LIGHTSPEED_PLATFORM_WINDOWS64
#define LIGHTSPEED_PLATFORM_WINDOWS

#elif defined(_WIN32)

#define LIGHTSPEED_PLATFORM_WINDOWS

#elif defined(__linux__)

#define LIGHTSPEED_PLATFORM_LINUX


#elif defined(_XBOX_)

#define LIGHTSPEED_PLATFORM_XBOX

#else

#error Unknown platform. Please define one of above macros to specify platform
#endif


#endif
