/*
 * dispatchThread.h
 *
 *  Created on: 15. 9. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_DISPATCHER_H_
#define LIGHTSPEED_MT_DISPATCHER_H_
#include "thread.h"
#include "../base/actions/dispatcher.h"

namespace LightSpeed {

class Dispatcher: public AbstractDispatcher {
public:
	Dispatcher();
	explicit Dispatcher(IRuntimeAlloc *allocator);
	explicit Dispatcher(ISleepingObject *observer);
	Dispatcher(ISleepingObject *observer, IRuntimeAlloc *allocator);
	~Dispatcher();

	virtual void dispatchAction(AbstractAction *action);

	bool sleep(const Timeout &tm, natural &reason);
	bool sleep(const Timeout &tm);

protected:

	virtual bool implSleep(const Timeout &tm, natural &reason);

	AbstractAction * volatile queue;
	IRuntimeAlloc *allocator;
	ISleepingObject *observer;

	void executeQueue();
	void cancelQueue();
	virtual IRuntimeAlloc &getActionAllocator();

};

///Implements thread with embedded dispatcher
/** Main benefit of this combination is that dispatcher can dispatch messages
 * while thread is waiting for an event. To dispatch messages, the thread just need
 * to call Thread::sleep() or Thread::deepSleep() function. Messages are dispatched in context
 * of the thread.
 *
 * @note Because dispatching messages can take a time, functions sleep() or deepSleep() can
 * actually return later than requested.
 *
 * Note, this feature affects just Thread::sleep, which is used in most of synchronization objects
 * exposed by the library LightSpeed (FastLock,Promise,SyncPt). Any other places, where thread is blocked, including
 * waiting on I/O, doesn't execute dispatching
 */
class DispatcherThread: public Dispatcher, public Thread {
public:
	DispatcherThread();
	DispatcherThread(IRuntimeAlloc *allocator);

	///runs the thread until finished.
	/**
	 * The function executes infinite loop of sleep() until canFinish() is signaled. Useful
	 * to start dispatching-only threads
	 */
	void run();

	///Enables or disables dispatching during sleep() (also affects run() function)
	/** By default, dispatcher starts with dispatching enabled. But dispatching is
	 * automatically disabled during dispatching, so calling sleep() under dispatching
	 * doesn't execute nested dispatching. However, you can still enable dispatching if it
	 * is intended. You can also disable dispatching, if you need to wait for precise time or avoid
	 * interruption.
	 *
	 * @param state new state of dispatching
	 * @return previous state
	 */
	bool enable(bool state);
private:

	bool impSleep(const Timeout &tm, natural &reason);
	bool alertable;
};


} /* namespace LightSpeed */

#endif /* LIGHTSPEED_MT_DISPATCHER_H_ */
