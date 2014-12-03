/*
 * linuxNetWaitingObj.cpp
 *
 *  Created on: 17.6.2012
 *      Author: ondra
 */

#include "linuxNetWaitingObj.h"
#include "../streams/netio_ifc.h"
#include "../interface.tcc"

namespace LightSpeed {

LinuxNetWaitingObj::LinuxNetWaitingObj():loadNext(false) {
	fdSelect.enableWakeUp(true);
}

LinuxNetWaitingObj::~LinuxNetWaitingObj() {

}

void LinuxNetWaitingObj::add(INetworkResource* rsrc, natural waitFor,
		natural timeout_ms) {

	natural i;
	int sckid;
	INetworkSocket &sck = rsrc->getIfc<INetworkSocket>();
	for (i = 0; (sckid = sck.getSocket(i)) != -1; i++) {
		fdSelect.set(sckid,waitFor,rsrc,timeout_ms);
	}

}

void LinuxNetWaitingObj::remove(INetworkResource* rsrc) {
	natural i;
	int sckid;
	INetworkSocket &sck = rsrc->getIfc<INetworkSocket>();
	for (i = 0; (sckid = sck.getSocket(i)) != -1; i++) {
		fdSelect.unset(sckid);
	}

}

const LinuxNetWaitingObj::EventInfo& LinuxNetWaitingObj::getNext() {
	loadNext = true;
	return peek();
}

const LinuxNetWaitingObj::EventInfo& LinuxNetWaitingObj::peek() const {
	if (loadNext) {
		do {
			const LinuxFdSelect::FdInfo &res = fdSelect.getNext();
			natural reason;
			if (fdSelect.isWakeUpRequest(res,reason)) {
				retValue.rsrc = 0;
				retValue.event = res.waitMask;
				break;
			} else if (res.data != 0) {
				retValue.event = res.waitMask;
				retValue.rsrc = reinterpret_cast<INetworkResource *>(res.data);
				fdSelect.set(res.fd,0,0,nil);
				break;
			}
		} while (true);

	}
	return retValue;
}

bool LinuxNetWaitingObj::hasItems() const {
	return fdSelect.hasItems();
}

bool LinuxNetWaitingObj::sleep(natural milliseconds) {
	return fdSelect.waitForEvent(milliseconds);
}

	void LinuxNetWaitingObj::wakeUp(natural reason) throw () {
		fdSelect.wakeUp(reason);
	}
}
/* namespace LightSpeed */
