#pragma once
#include "winsocket.h"

namespace LightSpeed {

template<typename Base>
WindowsSocketResource<Base>::~WindowsSocketResource() {
	if (!noclose) {
		closesocket(sock);
	}
}


static inline struct ::timeval millisecToTimeval(natural millisec) {
	struct timeval val;
	if (millisec == naturalNull) {
		val.tv_sec = ~0;
		val.tv_usec = 0;
	} else {
		val.tv_sec = long(millisec / 1000);
		val.tv_usec = long((millisec % 1000) * 1000);
	}
	return val;
}


template<typename Base>
natural WindowsSocketResource<Base>::doWait(natural waitFor, natural timeout) const
{
	if (waitFor == 0) waitFor = waitForInput;

	FD_SET inputSet,outputSet,exceptSet;
	FD_SET *ptrin,*ptrout,*ptrexc;
	if (waitFor & waitForInput) {
		FD_ZERO(&inputSet);
		FD_SET(sock,&inputSet);
		ptrin = &inputSet;
	} else {
		ptrin = 0;
	}
	if (waitFor & waitForOutput) {
		FD_ZERO(&outputSet);
		FD_SET(sock,&outputSet);
		ptrout = &outputSet;
	} else {
		ptrout = 0;
	}
	if (waitFor & waitForException) {
		FD_ZERO(&exceptSet);
		FD_SET(sock,&exceptSet);
		ptrexc = &exceptSet;
	} else {
		ptrexc = 0;
	}

	struct timeval tm = millisecToTimeval(timeout);
	int res = select(int(sock+1),ptrin,ptrout,ptrexc,timeout==naturalNull?0:&tm);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,WSAGetLastError(),"select failed");
	if (res == 0)
		return waitTimeout;
	natural out = 0;
	if (ptrin && FD_ISSET(sock,ptrin))
		out |= waitForInput;
	if (ptrout && FD_ISSET(sock,ptrout))
		out |= waitForOutput;
	if (ptrexc && FD_ISSET(sock,ptrexc))
		out |= waitForException;
	return out;
}

}