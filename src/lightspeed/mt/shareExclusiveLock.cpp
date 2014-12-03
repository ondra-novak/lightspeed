/*
 * rwlock.cpp
 *
 *  Created on: 9.5.2011
 *      Author: ondra
 */


#include "../base/types.h"
#include "shareExclusiveLock.h"

namespace LightSpeed {


void ShareExclusiveLock::lockAsync(SharedSlot& slot) {
	Synchronized<FastLock> _(lk);

	slot.owner = this;
	if (syncPoint.empty() && locked == 0) {
		slot.wakeUp(0);
	} else {
		syncPoint.add(slot);
	}
}

void ShareExclusiveLock::lockCancel(Slot& slot) {
	syncPoint.remove(slot);
}

void ShareExclusiveLock::lockAsync(ExclusiveSlot& slot) {
	Synchronized<FastLock> _(lk);
	slot.owner = this;
	if (readerCount == 0 && locked == 0) {
		slot.wakeUp(0);
	} else {
		syncPoint.add(slot);
	}
}

void ShareExclusiveLock::unlock() {
	Synchronized<FastLock> _(lk);
	if (locked) {
		locked = 0;
		while (!locked) {
			syncPoint.notifyOne();
		}
	} else {
		readerCount--;
		if (readerCount == 0) {
			syncPoint.notifyOne();
		}
	}
}


void ShareExclusiveLock::SharedSlot::wakeUp(natural ) throw() {
	owner->readerCount++;
}

void ShareExclusiveLock::ExclusiveSlot::wakeUp(natural ) throw() {
	owner->locked = 1;
}

}

#if 0
void ShareExclusiveLock::add(SyncSlotBase &slot) {

	Lock _(SyncPt::inrlk);
	if (exclusiveOwner == 0 && !exclusiveWaiting()) {
		if (slot.getThreadId() == 0) {
			rdCount++;
			slot.setSignaled();
		} else if (rdCount == 0) {
			exclusiveOwner = slot.getThreadId();
			slot.setSignaled();
		} else {
			SyncPt::add(slot);
		}
	} else if (exclusiveOwner == slot.getThreadId() ||
			(slot.getThreadId() == 0 && ThreadId::current().asAtomic() == exclusiveOwner)){
		slot.setSignaled();
		rdCount--;
	} else {
		SyncPt::add(slot);
	}

}


bool ShareExclusiveLock::lockShared(const Timeout &tm , WaitInterruptMode md )
{
	SyncSlotExclusive s(*this);
	return s.wait(tm,md);
}
bool ShareExclusiveLock::lockExclusive(const Timeout &tm , WaitInterruptMode md )
{
	SyncSlotShared s(*this);
	return s.wait(tm,md);
}



void ShareExclusiveLock::unlock()
{
	Lock _(SyncPt::inrlk);

	if (exclusiveOwner) {
		ThreadId id(exclusiveOwner);
		if (id == ThreadId::current()) {
			if (rdCount < 0) rdCount++;
			else exclusiveOwner = 0;
		}
	} else {
		if (rdCount > 0) rdCount--;
	}
	releaseReaders();
	if (first && first->getThreadId() != 0 && rdCount == 0) {
		SyncSlotBase *x = notifyOne();
		exclusiveOwner = x->getThreadId();
	}
}

void ShareExclusiveLock::leaveExclusive() {
	Lock _(SyncPt::inrlk);

	if (exclusiveOwner) {
		ThreadId id(exclusiveOwner);
		if (id == ThreadId::current()) {
			exclusiveOwner = 0;
			rdCount=-rdCount;
			releaseReaders();
		}
	}
}

bool ShareExclusiveLock::remove(SyncSlotBase & slot)
{
	Lock _(SyncPt::inrlk);
	bool r = SyncPt::remove(slot);
	if (slot) {
		unlock();
	}
	releaseReaders();
	return r;
}


bool ShareExclusiveLock::isLocked() const {
	Lock _(SyncPt::inrlk);
	return exclusiveOwner || rdCount;
}

ShareExclusiveLock::ShareExclusiveLock()
	:exclusiveOwner(0),rdCount(0)
{
}



SyncSlotShared::SyncSlotShared(ISleepingObject &wobj, ShareExclusiveLock &syncPt)
	:SyncSlot(wobj,syncPt){
		ssb.setThreadId(0);
}

SyncSlotShared::SyncSlotShared(ShareExclusiveLock &syncPt)
	:SyncSlot(syncPt){
		ssb.setThreadId(0);
}

SyncSlotExclusive::SyncSlotExclusive(ISleepingObject &wobj, ShareExclusiveLock &syncPt)
	:SyncSlot(wobj,syncPt){
		ssb.setThreadId(ThreadId::current().asAtomic());
}

SyncSlotExclusive::SyncSlotExclusive(ShareExclusiveLock &syncPt)
	:SyncSlot(syncPt){
		ssb.setThreadId(ThreadId::current().asAtomic());
}

bool ShareExclusiveLock::exclusiveWaiting() const
{
	return first != 0 || added != 0;

}



void ShareExclusiveLock::releaseReaders()
{
	reorder();
	while (first && first->getThreadId() == 0) {
		rdCount++;
		notifyOne();
	}
}



void ShareExclusiveLock::lock(LockMode md) {
	bool r;
	if (md == shared) r = lockShared(nil);
	else r = lockExclusive(nil);
	if (!r) throw ;//todo throw exception

}
#endif
