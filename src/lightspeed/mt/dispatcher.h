#pragma once
#include "../base/actions/idispatcher.h"
#include "../base/memory/pointer.h"
#include "gate.h"
#include "thread.h"
#include "../base/memory/poolalloc.h"
#include "../base/memory/sharedPtr.h"
#include "../base/containers/queue.h"

namespace LightSpeed {
	

	class Dispatcher : public IDispatcher {
	public:
		Dispatcher();
		~Dispatcher();

		virtual void dispatch(AbstractAction *action);

		///starts dispatcher thread
		virtual void run();
		///signals dispatcher to quit dispatching
		virtual void quit();
		///synchronizes current thread with dispatcher
		virtual void join();

	protected:
		virtual IRuntimeAlloc & getActionAllocator();
		virtual void promiseRegistered(PPromiseControl ppromise);
		virtual void promiseResolved(PPromiseControl ppromise);

		///Called to notify thread about new message		
		/**
		 @note if there are messages, function is not called, because it expects, that thread will
		  check messages before it goes to sleep 
		 */
		virtual void onNewMessage() throw();

		///Called, when thread has no work to do
		/**
		  @param cnt idle counter - each time function is called, argument is increased. When a new
		   message is dispatched, counter is reset. Function can use this argument to count how long
		   there were no message dispatched
		  @return timeout-value which is passed to the sleep() function. Sleep can be interrupted by
		  a new message or by any other wakeUp() request. Function onNewMessage() can also interrupt 
		  the sleep (event if there were no message enqueued)

		  @note during onIdle, the internal lock is held, so dispatcher will block any attempt to
		  dispatch a message. If you need to dispatch messages during the onIdle, you have to temporary
		  unlock that lock. Example:
		  @code
		  Timeout onIdle(natural cnt) {
			SyncReleased<FastLock> _(this->lock);
			//... do anything here while the dispatcher accept messages ...
			return Timeout(...);
		  }
		  @encode
		  */
		virtual Timeout onIdle(natural cnt);


		void finishRun() throw();
		AbstractAction * getNextAction();
		void rejectQueue();
		Pointer<Thread> currentThread;
		Gate running;
		bool needstop;
		typedef AllocPointer<AbstractAction> PAction;
		typedef Queue<PAction> MQueue;

		MQueue queue;
		natural head;
		FastLock lock;

		PoolAlloc alloc;

	};
}
