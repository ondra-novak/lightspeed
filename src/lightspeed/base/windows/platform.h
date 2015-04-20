#ifndef  _LIGHTSPEED_WINDOWS_PLATFORM_H_
#define  _LIGHTSPEED_WINDOWS_PLATFORM_H_


#include <stdint.h>

#if defined(__x86_64__) || defined(_M_X64)
#define LIGHTSPEED_ENV_64BIT
#else
#define LIGHTSPEED_ENV_32BIT
#endif


#include "winpch.h"

#endif /*PLATFORM_*/

