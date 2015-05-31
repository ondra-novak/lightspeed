/*
 * syncPt.cpp
 *
 *  Created on: 20.4.2011
 *      Author: ondra
 */

#include "syncPt.h"
#include "timeout.h"
#include "thread.h"
#include "../base/sync/synchronize.h"



namespace LightSpeed {


bool SyncPt::wait(Slot &slt,const Timeout& tm, WaitInterruptMode mode) {
	switch (mode) {
		case uninterruptible: while (!slt && !tm.expired()) Thread::sleep(tm); return slt;
		case interruptOnExit: while (!slt && !tm.expired() && !Thread::canFinish()) Thread::sleep(tm); return slt;
		case interruptable: if (!slt && !tm.expired()) Thread::sleep(tm); return slt;
	}
	return false;
}

bool SyncPt::add(Slot& slot) {
	//check, whether can be slotted
	if (slot.next || slot.signaled || slot.charged) return false;

	//wait for lock
	Synchronized<FastLock> _(queueLock);
	add_lk(slot);

	//slot can be signaled from now
	//return success
	return true;
}

bool SyncPt::remove(Slot& slot) {
	if (slot.charged) {
		Synchronized<FastLock> _(queueLock);
		//pick queue
		Slot *q = curQueue;
		//if queue is empty
		if (q == 0) {
			//can't remove
			return false;
		//if first item is slot
		}else if (q == &slot) {
			//set queue-s new head
			curQueue = slot.next;
			//slot removed
			return true;
		} else {
			//find slot
			while (q->next != 0 && q->next != &slot)
				q = q->next;

			bool suc;
			if (q->next == &slot) {
				//remove slot if found
				q->next = slot.next;
				//removed
				suc = true;
			} else {
				//failed
				suc = false;
			}

			return suc;
		}
	} else {
		//slot is not charged, false
		return true;
	}
}


SyncPt::SyncPt():curQueue(0),order(queue) {}
SyncPt::SyncPt(Order order):curQueue(0),order(order) {}


SyncPt::~SyncPt()
{
	//acquire lock to ensure that internal state of object is stable
	queueLock.lock();
	//lock is kept till destroy. Because it is fastlock, it is allowed to destroy owned lock
	//no resource is left behind. 
	//
	//Using SyncPt during destroying is forbidden and may lead to unpredictable result.
	//we will not handle this situation anyway
	//
	//But SyncPt can be destroyed before it finishes its internal operation
	//acquiring the lock handles this situation.
}

bool SyncPt::notifyOne() {
	Synchronized<FastLock> _(queueLock);
	Slot *p = popSlot_lk();
	if (p == 0) return false;
	p->notify(0);
	return true;
}

bool SyncPt::notifyAll() {
	Synchronized<FastLock> _(queueLock);
	bool suc = false;
	Slot *p = curQueue;
	curQueue = 0;
	while (p) {
		Slot *q = p;
		p = p->next;
		q->notify();
		suc = true;
	}
	return suc;
}

SyncPt::Slot* SyncPt::popSlot_lk() {
	//called under lock!
	//get first item
	Slot *p = curQueue;
	//in case that queue is not empty
	if (p != 0) {
		//in case, that ordering is queue and there is more items
		if (order == queue && p->next != 0) {
			Slot *q;
			//walk all slots
			do {
				q = p;
				p = p->next;
				//find end of chain
			} while (p->next != 0);
			//remove ending slot
			q->next = 0;
			//p contains found item
		} else {
			//stack mode, remove top item
			curQueue = p -> next;
			//reset next value
			p -> next = 0;

		}
	}
	//return found item
	return p;
}

void SyncPt::notifySlot_lk( Slot *slot )
{
	slot->notify();
}

void SyncPt::add_lk( Slot &slot )
{
	//set charged
	slot.charged = true;
	//insert as top of queue
	slot.next = curQueue;
	curQueue = &slot;
}



}

