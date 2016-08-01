#ifndef  _LIGHTSPEED_WINDOWS_PLATFORM_H_
#define  _LIGHTSPEED_WINDOWS_PLATFORM_H_

#if _MSC_VER < 1900
#include "msstdint.h"
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define LIGHTSPEED_ENV_64BIT
#else
#define LIGHTSPEED_ENV_32BIT
#endif

#ifndef LIGHTSPEED_DEPRECATED
#define LIGHTSPEED_DEPRECATED __declspec(deprecated) 
#endif

#if _MSC_VER > 1800
#define LIGHTSPEED_ENABLE_CPP11
#endif

#include "winpch.h"

#endif /*PLATFORM_*/

