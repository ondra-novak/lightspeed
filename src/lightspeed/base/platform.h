/** @file
 * Detects current platform a redirs headers to the right directory
 * 
 * To access platform headers, define macro LIGHTSPEED_SEARCH_HEADER and
 * include platform.h
 */

/* Windows platform **/
#if defined LIGHTSPEED_SEARCH_HEADER
#if defined(_WIN32) || defined(_WIN64)
#include "windows\platform.h"
#else
#include "linux/platform.h"
#endif
#undef LIGHTSPEED_SEARCH_HEADER
#else
#error You have to specify LIGHTSPEED_SEARCH_HEADER before platform.h is included
#endif
