/*
 * IOSleep.h
 *
 *  Created on: 20.12.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_LINUX_IOSLEEP_H_
#define LIGHTSPEED_MT_LINUX_IOSLEEP_H_

#include "../../mt/sleepingobject.h"
#include "../../mt/timeout.h"

namespace LightSpeed {

class INetworkResource;

class IOSleep: public ISleepingObject {
public:

	IOSleep();
	~IOSleep();

	enum SleepResult {
		data,
		waken,
		timeout
	};

	SleepResult sleep(INetworkResource *nres, Timeout tm);

	virtual void wakeUp(natural reason = 0) throw() ;


	natural getReason() const {return reason;}

protected:

	int fdnotify;
	int fdcancel;
	natural reason;
};

} /* namespace LightSpeed */
#endif /* LIGHTSPEED_MT_LINUX_IOSLEEP_H_ */
