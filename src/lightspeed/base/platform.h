
#if defined(_WIN32) || defined(_WIN64)
#define LIGHTSPEED_PLATFORM_WINDOWS
#include "windows\platform.h"
#else
#define LIGHTSPEED_PLATFORM_LINUX
#include "linux/platform.h"
#endif
