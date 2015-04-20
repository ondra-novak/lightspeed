#pragma once
#include "linsocket.h"

namespace LightSpeed {

template<typename Base>
LinuxSocketResource<Base>::~LinuxSocketResource() {
	if (!noclose) {
		close(sock);
	}
}


template<typename Base>
natural LinuxSocketResource<Base>::doWait(natural waitFor, natural timeout) const
{
	return safeSelect(sock,waitFor,timeout);
}

}
