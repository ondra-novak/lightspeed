#pragma once
#include "thread.h"
#include "spinlock.h"
#include "notifier.h"
#include "../base/actions/msgQueue.h"
#include "../base/actions/scheduler.h"

namespace LightSpeed {


	///Base class to support MsgThread and SchedulerThread
	/** Class contains some common methods and procedures */
	class MsgThreadBase: public Thread {
	public:

		///ctor
		MsgThreadBase();
		///dtor - stops the thread
		~MsgThreadBase();
		///Enables manual start
		/** Manual start need to start thread explicitly
		 * 
		 * Default value is false and thread starts on arrival of the first message. When this happen
		 * this option is changed to true preventing automatic start after thread is stopped and
		 * you have to 
		 */
		void setManualStart(bool manstart);

	protected:
		SpinLock startLock;
		bool manStart;

		void notify();
		void autoStart();
		virtual void worker() = 0;

	private:
		MsgThreadBase(const MsgThreadBase &);
		MsgThreadBase& operator=(const MsgThreadBase &);

	};


	class MsgThread: public MsgQueue, public MsgThreadBase {
	public:

		MsgThread();
		MsgThread(IRuntimeAlloc &rtAlloc);

		static MsgThread &current();
		static MsgThread *currentPtr();

	protected:
		virtual void notify();		

		virtual void worker();
		///Perform idle tasks
		/** Idle task can be performed when there is
		 *  no messages in the queue.
		 * @param cntr countains count of idle cycles since
		 *    last message has arrived, while first cycle has
		 *    number 0
		 * @return count of milliseconds till next idle cycle until
		 *    message arrives (which resets counter). Function can
		 *    return 0 to check only queue without waiting. Return
		 *    value naturalNull causes, that no more idle cycles
		 *    will be generated.
		 */
		virtual natural onIdle(natural cntr);
	};

/*	static void setThreadMsgQueue(MsgQueue *msgQueue);
	static MsgQueue *getThreadMsgQueue(Thread &thr);
*/

	class SchedulerThread: public Scheduler, public MsgThreadBase {
	public:

		SchedulerThread();
		SchedulerThread(IRuntimeAlloc &rtAlloc);

		static SchedulerThread &current();
		static SchedulerThread *currentPtr();

	protected:
		virtual void notify();
		virtual void worker();
		virtual natural getCurTime() const;

		natural getSystemTime() const;
	};

}
