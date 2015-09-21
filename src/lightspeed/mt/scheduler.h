/*
 * scheduler.h
 *
 *  Created on: 16. 9. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_SCHEDULER_H_
#define LIGHTSPEED_MT_SCHEDULER_H_
#include "dispatcher.h"
#include "../base/containers/sort.h"
#include "../base/actions/ischeduler.h"

namespace LightSpeed {


///Scheduler is dispatcher, that also allows to schedule actions
/**
 * Scheduler is thread+dispatcher, where other task of the thread is resolve
 * promises scheduled on specified time.
 *
 * To use scheduler, you can call schedule() which returns promise, which becomes
 * resolved, once specified timeout expires. If you want to call
 * a function on that time, you can attach that function to the promise using Promise::then() or Promise::thenCall
 *
 *  Promise can be also rejected when scheduler is exiting without completting scheduled tasks.
 */
class Scheduler: public DispatcherThread, public IScheduler {
public:
	Scheduler();
	~Scheduler();

	///Schedule a task
	/**
	 * @param tm timeout specifies, when task should be executed
	 * @return promise, which is resolved at specified time. You can attach the task to the promise.
	 */
	virtual Promise<void> schedule(Timeout tm);

	///Starts the scheduler.
	/**
	 * By default, scheduler is constructed inactive, you have to call this function to assign
	 * a thread to the scheduler. You should call this function in the thread (so at least through
	 * Thread::start())
	 *
	 */
	void run();

	///Orders the scheduler to stop
	/**
	 * @param async if true, function just places the order and returns immediatelly without
	 * waiting to completion. You can wait throuh getJoinObject()
	 */
	void stop(bool async = false);


	///Returns gate object which is opened, when scheduler is not running
	Gate &getJoinObject();

protected:

	class Event: public ComparableLess<Event> {
	public:

		Event(Timeout tm, Promise<void>::Result res);

		bool expired(const SysTime &tm) const;
		const Timeout &getTimeout() const;
		Promise<void>::Result getPromise() const;


	protected:
		Timeout tm;
		Optional<Promise<void>::Result> res;

		friend class ComparableLess<Event>;

		bool lessThan(const Event &ev) const;
	};

	void addEvent(const Event &ev);
	Timeout getNextWait() const;
	bool fireExpired();

	typedef AutoArray<Event> EventMap;
	typedef HeapSort<EventMap> EventHeap;

	EventMap eventMap;
	EventHeap eventHeap;
	bool canFinish;
	Gate running;

	void cancelAllEvents();

private:
	Scheduler(const Scheduler &other);
};

} /* namespace LightSpeed */

#endif /* LIGHTSPEED_MT_SCHEDULER_H_ */
