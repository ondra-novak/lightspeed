/*
 * sleepingobject.h
 *
 *  Created on: 29.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_IWAITINGOBJECT_H_
#define LIGHTSPEED_MT_IWAITINGOBJECT_H_

#include "../base/compare.h"

namespace LightSpeed {

///Objects that are able to wait for an event
/**

 */
class ISleepingObject {
public:

	///Notifies about event and wake ups the waiting object
	/**
	 * If this object is Thread, function resumes the sleeping thread. If
	 * thread doesn't sleep, it records this event and prevent sleeping
	 * on next sleep() call.
	 *
	 * If object is not Thread, behavior depends on implementation. Function
	 * is designed to send notification between the threads.
	 *
	 * Function should not process long-term tasks. It should finish it work
	 * as soon as possible. In most of cases function releases a semaphore
	 * or unblocks the event object.
	 *
	 * @param reason optional argument associated with the request. It can
	 * carry information about reason of wake up. If not specified, it has
	 * value zero. Also target object can ignore this value when it don't need it.
	 *
	 * @note threads should not be waken up with reason, because storing
	 * and carrying reason is not MT safe and it can be lost during processing.
	 */
	virtual void wakeUp(natural reason = 0) throw() = 0;

	///Virtual destructor need to correct destruction through this interface
	virtual ~ISleepingObject() {}
};


template<typename T, void (T::*func)(natural) throw()>
class WakeUpListener: public ISleepingObject {
public:
	WakeUpListener(T &owner):owner(owner) {}
	virtual void wakeUp(natural reason = 0) throw() {(owner.*func)(reason);}
protected:
	T &owner;
};

///IWaitingObject is an old name
typedef ISleepingObject IWaitingObject;

}  // namespace LightSpeed

#endif /* LIGHTSPEED_MT_IWAITINGOBJECT_H_ */
