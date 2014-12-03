/*
 * linuxNetWaitingObj.h
 *
 *  Created on: 17.6.2012
 *      Author: ondra
 */

#ifndef LIGHTSPEED_NETWORK_LINUX_LINUXNETWAITINGOBJ_H_
#define LIGHTSPEED_NETWORK_LINUX_LINUXNETWAITINGOBJ_H_

#pragma once

#include "../streams/netio_ifc.h"
#include "linuxFdSelect.h"

namespace LightSpeed {

class LinuxNetWaitingObj: public INetworkWaitingObject {
public:
	LinuxNetWaitingObj();
	virtual ~LinuxNetWaitingObj();

	virtual void add(INetworkResource *rsrc, natural waitFor, natural timeout_ms);
	virtual void remove(INetworkResource *rsrc);
	virtual const EventInfo &getNext();
	virtual const EventInfo &peek() const;
	virtual bool hasItems() const;
	virtual bool sleep(natural milliseconds);
	virtual void wakeUp(natural reason) throw();

protected:

	mutable EventInfo retValue;
	mutable LinuxFdSelect fdSelect;
	mutable bool loadNext;
};

} /* namespace LightSpeed */
#endif /* LIGHTSPEED_NETWORK_LINUX_LINUXNETWAITINGOBJ_H_ */
