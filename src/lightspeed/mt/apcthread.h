/*
 * apcserver.h
 *
 *  Created on: 23.6.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_APCSERVER_H_
#define LIGHTSPEED_MT_APCSERVER_H_

#include "apc.h"
#include "thread.h"

namespace LightSpeed {


	///APC accepting thread
	/** This thread supports APC processing (APC - Asynchronous procedure call)
	 *
	 * APC is represented by object AbstractAPC. Caller must create new
	 * object that inherits this class and implement method AbstractAPC::run()
	 *
	 * APCThread is ordinary thread, until another thread enqueues an APC
	 * message through execute(). APC is automatically processed during next
	 * sleep(). If there are more APC enqueued, all of them are processed
	 * regardless to argument of sleep function.
	 *
	 * An APC procedure should not interfer thread's context, it should
	 * run as interrupt routine. That because APC can be executed on
	 * any sleep(), including waiting on locks, such a FastLock. So main benefit
	 * of APC is that APC routine can access, and modify threads local
	 * variables without need additial synchronization. On some operation
	 * systems, there are objects bind with thread that has been created by it.
	 * If other thread want to access them, it need to make it through
	 * APC message.
	 *
	 * APC message can be allocated on the stack or on the heap. It needn't be destroyed
	 * until it is processed. Enqueued message CANNOT BE REMOVED
	 * from the queue. If message is allocated on the heap, it has to
	 * perform cleaning by self. Best way to do this is include @b delete @b this
	 * as last operation of APC routine.
	 *
	 * When thread is executing APC routine, sleep() function is no more handling
	 * other APC's until this is enabled by enableAPC(). Thread
	 * itself can disable APC while it entering into some critical place.
	 *
	 * @note After all APCs are processed, function sleep() exits with
	 * false, regardless on timeout settings. Function sleep() can be also interrupted when
	 * APC is queued while APCs are disabled. All code should not depend
	 * on sleep's result, it should always use additional notifiers (Notifier)
	 * to ensure, that expected notification already arrived.
	 *
	 * @note APCs can be enqueued event if thread is not started yet or it is already
	 * terminated. Destuctor of the APCThread calls AbstractAPC::reject() in
	 * the context of thread destroying the APCThread object.
	 *
	 *
	 */
	class APCThread: public Thread, public IAPCServer {
	public:
		APCThread();

		///Enables or disables APC
		/**
		 * @param enable true to enable APC, false to disable APC.
		 * 	During execution of an APC, other APCs are disabled until current
		 * 	APC returns. However, APC routine can enable and disable other
		 * 	APCs as it require. Settings is always reset between each
		 * 	APC call.
		 */
		void enableAPC(bool enable);
		///Enqueues new APC into the thread
		/** To prevent API disambignuality, function execute() has been
		 * put unavailable into "protected" section. You should call
		 * queueAPC() directly on APCThread. If you need to call execute(),
		 * first convert reference to IAPCServer's reference.
		 *
		 * @param apc Object containing APC routine to be executued in
		 *  the context of this APCThread
		 */
		void queueAPC(AbstractAPC *apc) {execute(apc);}


	protected:

		virtual bool execute(AbstractAPC *apc);


		class APCh: public AbstractAPCServer {
		public:
			APCThread *owner;
			APCh(APCThread *owner):owner(owner) {}
			virtual void onExecute(AbstractAPC *p) throw ();
		};

		void onExecute(AbstractAPC *p) throw();
		virtual bool impSleep(const Timeout &tm, natural &reason);
		bool apc_enabled;
		APCh apcImpl;


	};

}



#endif /* LIGHTSPEED_MT_APCSERVER_H_ */
