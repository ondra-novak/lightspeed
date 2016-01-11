#include "dispatcher.h"
#include "../base/containers/sort.tcc"
#include "scheduler.h"
#include "../base/containers/autoArray.tcc"
#include "../base/actions/promise.tcc"

namespace LightSpeed {


	Scheduler::Scheduler() :pqheap(pqueue)
	{

	}

	void Scheduler::schedule(const Timeout &tm, const Promise &promise)
	{
		if (tm.isInfinite()) throw InvalidParamException(THISLOCATION, 0, "Cannot use infinity time");
		Synchronized<FastLock> _(lock);
		pqueue.add(QueueItem(tm, promise));
		pqheap.push();
		onNewMessage();
	}

	LightSpeed::Timeout Scheduler::onIdle(natural)
	{
		if (pqheap.empty()) return nil;

		SysTime curTime = SysTime::now();
		QueueItem &itm = pqueue(0);
		if (itm.tm.expired(curTime)) {			
			Promise p = itm.promise;
			pqheap.pop();
			pqueue.trunc(1);
			SyncReleased<FastLock> _(lock);
			itm.promise.resolve();
			return Timeout(0);
		}
		else {
			return itm.tm;
		}		
	}

	bool Scheduler::CmpQueue::operator()(const QueueItem &a, const QueueItem &b) const
	{
		return a.tm > b.tm;
	}

}
