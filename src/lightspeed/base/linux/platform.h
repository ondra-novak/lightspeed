#ifndef  _LIGHTSPEED_LINUX_PLATFORM_H_
#define  _LIGHTSPEED_LINUX_PLATFORM_H_

#include <stdint.h>

#if defined(__x86_64__) || defined(_M_X64)
#define LIGHTSPEED_ENV_64BIT
#else
#define LIGHTSPEED_ENV_32BIT
#endif

#ifndef LIGHTSPEED_DEPRECATED
#ifdef __GNUC__
#define LIGHTSPEED_DEPRECATED __attribute__((deprecated))
#else
#define LIGHTSPEED_DEPRECATED
#endif
#endif


#endif /*PLATFORM_H_*/

