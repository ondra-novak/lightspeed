/*
 * notifier.h
 *
 *  Created on: 1.10.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_NOTIFIER_H_
#define LIGHTSPEED_MT_NOTIFIER_H_
#include "sleepingobject.h"
#include "atomic.h"
#include <assert.h>

namespace LightSpeed {


	class Timeout;

	///Proxy to ISleepingObject allowing to separate each notification incoming to the thread
	/**Object Notifier can be used in synchronization instead the Thread.
	 * Object is created as 'nonsignaled'. Once other thread calls wakeUp, it becomes
	 * signaled and forwards notification to the thread allowing it to pick
	 * and check state of this object,
	 *
	 * You can easy implement waiting for two or more synchronization objects at once.
	 * Just create multiple notifiers and each register with different synchronization
	 * object. Then order the thread to sleep. Once an event is detected,
	 * thread is waken up and can check signaled status of each notifier.
	 *
	 */
	class Notifier: public ISleepingObject {
	public:
		///Constructs notifier bound with current thread
		/**
		 * Easiest way to construct notifier, note that its automatically
		 * binds itself with current thread. Be careful, when you
		 * creating objects in different thread.
		 */
		Notifier();
		///Constructs notifier bound with specified thread or another notifier
		/**
		 * @param forward reference to object (Thread, Notifier, anything
		 * which implementing ISleepingObject) which receives notification
		 * through this notifier
		 */
		Notifier(ISleepingObject &forward):reason(0),signaled(0),forward(&forward) {}

		///Receives notification
		/**
		 * Notification is received, object is becoming signaled and notification
		 * is forwarded to the target sleeping object.
		 *
		 * @param reason reason of wakeUp, any user value. It is optional
		 * and default value is zero. Reason is also stored with notifier,
		 * In contrast with Thread, there is less possibility of race
		 * condition when reason is used with notifier so it can be sometime
		 * helpful carry some user data with notification.
		 */
		virtual void wakeUp(natural reason = 0) throw() {
//#ifdef _DEBUG
			if (((natural)readAcquire(&signaled)) > 1) debugBreak();
//#endif		
			//store forward - this object can disappear during processing
			ISleepingObject *fw = forward;
			//store reason first
			this->reason = reason;
			//make object signaled - now thread can continue
			writeRelease(&signaled,1);
			//if it sleeps, wake up it with reason.
			fw->wakeUp(reason);
		}

		///Directly tests whether notifier is signaled
		/**
		 * @retval true notifier is signaled
		 * @retval false notifier is not signaled
		 */
		operator bool() const {return readAcquire(&signaled) == 1;}

		///Returns inverted result of signaled state
		/**
		 * @retval true notifier is not signaled
		 * @retval false notifier is signaled
		 */
		bool operator !() const {return readAcquire(&signaled) == 0;}

		///Resets signaled state
		/**
		 * @note ensure, that object is really signaled before reset
		 * Never call reset() on non signaled object, because it can
		 * be signaled before reset() causing loose of event notification
		 */
		void reset() {
			writeRelease(&signaled,0);
		}

		///stored reason
		/** Notifier will store a reason value here. This is public member, you can
		 * read or write it without limitation
		 */
		natural reason;

		///Retrieves forward object
		/**
		 * @return object receiving notification forwarded by this notifier
		 */
		ISleepingObject *getForward() const {return forward;}

		///Perform wait to notification
		/**
		 * Function calls Thread::sleep() until this object is signaled
		 * @param tm Defines timeout
		 * @param wakeOnExit specify true, when waiting should be terminated because canFinish flag
		 * of current thread is set. Set false if you don't want to take care of this flag.
		 * @retval true, object is signaled
		 * @retval false, failure, timeout of finish flag.
		 */
		bool wait(const Timeout &tm, bool wakeOnExit = false);

#ifdef _DEBUG
		~Notifier() {
			writeRelease(&signaled, 0xFEEEFEEE);
		}
#endif

	protected:
		atomic signaled;
		ISleepingObject *forward;


	};

}

#endif /* LIGHTSPEED_MT_NOTIFIER_H_ */
