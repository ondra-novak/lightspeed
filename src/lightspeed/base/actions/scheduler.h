#pragma once

#include "msgQueue.h"
#include "../memory/dynobject.h"

namespace LightSpeed {

///Message queue extended with scheduler
/**
 * Scheduler allows enqueue message with time information
 * then function pumpMessage processes message expiring at specified
 * time. You have to set current time by calling setCurTime function
 *
 * Scheduler still can process standard messages which
 * are treat as message with current time stamp. You can also
 * enqueue priority messages which can be processed as soon as possible
 *
 * 
 */
 
class Scheduler: public MsgQueue {
public:
	
	///declaration of scheduled event
	/** its inherit AbstractMsg only adds natural number 
	  which has meaning of time (in milliseconds in many cases)
	  */
	class AbstractEvent: public AbstractMsg {
	public:

		///time of event (in milliseconds recomended)
		natural time;		

	};
	Scheduler();
	Scheduler(IRuntimeAlloc &alloc);

	///enqueues message with the time
	/**
	 * @param fn function to carry out
	 * @param interval in milliseconds relative to current time
	 */
	 
	template<typename Fn>
	AbstractMsg *scheduleFnCall(Fn fn, natural interval);

	///enqueues message with the time
	/**
	 * @param fn function to carry out
	 * @param reject function to carry out when message is rejected
	 * @param interval in milliseconds relative to current time
	 */
	template<typename Fn1, typename Fn2>
	AbstractMsg *scheduleFnCall(Fn1 fn, Fn2 reject, natural interval);

	void schedule(AbstractEvent *ev, natural interval);

	///retrieves current time
	/** Function is abstract. Implementation should define time units and function
	 * that is able to retrieve current time. It can be read from variable or retrieved from
	 * system time.
	 * @return current time value
	 *
	 * @note Function is marked as MT Safe, because it is called from various threads. If you
	 * implementation accessing shared resources, it have to use proper synchronization
	 * - for example locks.
	 */
	virtual natural getCurTime() const = 0;

protected:
	

	///Function prepares queue
	/**
	 * Other threads can add messages to the queue, which
	 * are invisible to worker thread until function prepareQueue
	 * is called. This allows to post messages to the queue
	 * without need to lock the queue. Function prepareQueue()
	 * takes all new messages atomically, and then it orders
	 * messages by the time. After ordering it merges messages
	 * to the current queue and returns.
	 *
	 * function is called automatically by pumpMessage(). 
	 */
	 
	virtual void prepareQueue();

	///Pumps messages carrying them inside worker thread
	/**
	 * @retval true message processed
	 * @retval false no more messages for current time
	 * @note do not forget to update current time after each
	 * message is processed. You can use pumpAllMessages(), but
	 * this function doesn't update current time.
	 *
	 */
	 
	virtual bool pumpMessage();
	
	///Retrieves how long worker thread should sleep for next scheduled message
	/**
	 * @retval 0 there is at least one message which can be carried out immediately
	 * @retval naturalNull there are no messages, worker thread can sleep for infinite period
	 * @retval other specified count of milliseconds to sleep
	 *
	 * @note worker thread must sleep interruptible to easy handle any new message
	 * Worker thread must implement function notify() to handle this
	 * situation and implement interruption of waiting
	 */
	 
	natural getWaitTime() const;
	natural getWaitTime();


private:
	AbstractMsg * sortList( AbstractMsg * p, natural curTime);
	AbstractMsg * mergeSort( AbstractMsg * a, AbstractMsg * b , natural curTime);
	CompareResult compareMsg( AbstractMsg * a, AbstractMsg * b , natural curTime);

};

template<typename Fn1, typename Fn2>
Scheduler::AbstractMsg * Scheduler::scheduleFnCall( Fn1 fn, Fn2 reject, natural interval )
{
	class MyMsg: public AbstractEvent, public DynObject {
	public:

		virtual void run() throw () {
			fn1();
		}
		virtual void reject() throw() {
			fn2();
		}

		Fn1 fn1;
		Fn2 fn2;

		MyMsg(Fn1 fn1, Fn2 fn2, natural tm):fn1(fn1), fn2(fn2) {
			time = tm;
		}

	};
	MyMsg *msg = new(rtAllocator) MyMsg(fn,reject,getCurTime()+interval);
	postMessage(msg);
	return msg;

}
template<typename Fn>
Scheduler::AbstractMsg *Scheduler::scheduleFnCall( Fn fnb, natural interval )
{
	class MyMsg: public AbstractEvent, public DynObject {
	public:

		Fn fnb;

		virtual void run() throw () {
			fnb();
		}


		MyMsg(Fn fnb, natural tm):fnb(fnb) {
			this->time = tm;
		}
	};
	MyMsg *msg = new(rtAllocator) MyMsg(fnb,getCurTime()+interval);	
	postMessage(msg);
	return msg;

}
}
