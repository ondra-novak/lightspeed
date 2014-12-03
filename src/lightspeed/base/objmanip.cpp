#include "objmanip.h"
#include <string.h>

namespace LightSpeed {


	void bincopy(void *target, const void *src, natural size) {
		memmove(target,src,size);
	}
}
