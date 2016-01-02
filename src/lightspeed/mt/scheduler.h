#pragma once
#include "dispatcher.h"
#include "..\base\containers\sort.h"

namespace LightSpeed {

	///Scheduler is object built on Dispatcher, which extends internal message pump by scheduling feature
	/**
	 * Scheduler works very simple. In specified time, scheduler resolves a promise, which has been
	   passed to the scheduler with the request. You can attach an observer to the promise, and so you 
	   are able to define function, what happened when the time comes.

	   Once the event is scheduled, it cannot be canceled, however, you can cancel the promise instead.
	   Promises are better suitable to solve various canceling issues, for example a race condition, when
	   program tries to cancel event, which has been already started, but not finished yet. Canceled 
	   promises are still resolved in time, but without observers, they do nothing. However, program should
	   not heavily schedule and cancel a lot of messages, because they are still kept in the queue that may
	   results of really large queue.

	   All scheduled events are executed in the dispatcher's thread. During the execution, the dispatcher
	   cannot process messages and/or scheduled events. If scheduled time is missed, the scheduler
	   executes all of them as soon as possible, once the current action returns from the processing.

	 */
	class Scheduler : public Dispatcher {
	public:

		///Create scheduler
		Scheduler();
		///scheduler an event
		/**
		  @param tm specifies time when the event will be scheduled. The Timeout object is used, so
		  you can plan the time relatively or absolute. The scheduler will not accept infinite timeout.
		  Using zero timeout causes, that promise will be resolved instantly or very soon, however note
		  that scheduler has lower priority then dispatcher (schedulers perform dispatching of all
		  dispatched messages before scheduled messages)
		  @param promise promise to resolve
		  @exception InvalidParamException timeout is set to infinity.
		*/		  		  
		void schedule(const Timeout &tm, const Promise<void> &promise);

	protected:

		typedef Promise<void> Promise;

		struct QueueItem {
			Timeout tm;
			Promise promise;
			QueueItem(Timeout tm, Promise promise) :tm(tm), promise(promise) {}
		};

		struct CmpQueue {
			bool operator()(const QueueItem &a, const QueueItem &b) const;
		};

		AutoArray<QueueItem> pqueue;
		HeapSort<AutoArray<QueueItem>, CmpQueue> pqheap;


		Timeout onIdle(natural cnt);





	};


}