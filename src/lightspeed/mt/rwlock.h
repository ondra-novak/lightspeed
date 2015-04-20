/*
 * fastrwlock.h
 *
 *  Created on: 14.10.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_RWLOCK_H_
#define LIGHTSPEED_MT_RWLOCK_H_
#include "fastlock.h"
#include "sleepingobject.h"
#include "threadMinimal.h"

namespace LightSpeed {

///Level lock - locks per level
/**@b RULES
 *
 * Lock is acquired at a specified level. Once at least one thread owns the lock
 * for given level, other threads can also own this lock if they acquire it
 * using the same level. Any thread that acquires it at different level must
 * wait until the lock is released. If there is a pending thread in the waiting
 * queue and another thread wants to acquire the lock using the same level as
 * current, it also falls to the queue, because first income thread has the
 * priority. 
 *
 * You can use Level lock to implement shared-exclusive-lock. Let's say that level 0
 * is shared level and any other level is exclusive level. Thread uses own threadId
 * as level so it will always request exclusive access because no other
 * thread has the same threadId.
 */
class LevelLock {
public:


	class Slot {
	public:
		Slot(ISleepingObject *n, atomicValue level):next(0),notify(n),level(level),signaled(false) {}

	private:
		Slot *next;
		ISleepingObject *notify;
		atomicValue level;
	public:
		bool signaled;

	friend class LevelLock;

	};


	LevelLock():qtop(0),qbot(0),curLevel(0),useCount(0) {}
	LevelLock(const LevelLock &) {}

	///locks for level
	/***
	 * @param level lock level (see rules)
	 */
	void lock(atomicValue level) {
		//create waiting slot
		Slot s(getCurThreadSleepingObj(),level);
		//register slot for waiting
		lockAsync(s);
		//wait until signaled
		while (!s.signaled) threadHalt();

	}
	///Perform asynchronous lock
	/**
	 * Set the slot and use lockAsync to start asynchronous locking. Once
	 * lock is acquired, signal is send through the slot to the specified waiting
	 * object (which should be current thread)
	 * If you want to stop waiting, you have to call cancelAsync() to remove
	 * waiting slot from the queue
	 *
	 * @param s waiting slot
	 */
	void lockAsync( Slot &s )
	{
		//enter critical section
		Synchronized<FastLock> _(protectInternals);
		//if queue is not empty or level is different and there are uses
		// in this case, current thread must wait in the queue
		if (qbot != 0 || (s.level != curLevel && useCount != 0)) {
			//if there is no queue, create it
			if (qtop == 0) {
				qbot = qtop = &s;			
			} else {
				//otherwise, append slot to queue
				qbot->next = &s;
				//move queue bottom 
				qbot = &s;
			}
		} else {
			//we acquired lock - mark signaled
			s.signaled = true;
			//update level
			curLevel = s.level;
			//increase use count
			useCount++;
		}
	}

	///Cancels asynchronous waiting for lock
	/**
	 * @param slot slot registered through lockAsync function
	 *
	 * Function calls unlock if removed slock is already signaled. Don't 
	 * check that value, once async is canceled, thread has no ownership
	 */
	void cancelAsync (Slot &s) {

		{
			//enter critical section
			Synchronized<FastLock> _(protectInternals);

			if (!s.signaled) {
				Slot *q = qtop;
				if (q == &s) qtop = q->next;
				else {
					while (q && q->next != &s) q = q ->next;
					if (q->next == &s) q->next =  s.next;
				}
			}
		}
		if (s.signaled) {
			unlock();
			s.signaled = false;
		}

	}

	///try lock
	bool tryLock(atomicValue level) {
		//lock critical section
		Synchronized<FastLock> _(protectInternals);
		//if queue is not empty or level is different and there are uses
		// in this case, tryLock fails
		if (qbot != 0 || (level != curLevel && useCount != 0)) 
			return false;
		// lock success, update level
		curLevel = level;
		//increase use count
		useCount++;
		//
		return true;
	}

	///unlock the lock
	void unlock() {
		//lock critical section
		Synchronized<FastLock> _(protectInternals);
		//decrease use count
		useCount--;
		//last use?
		if (useCount == 0) {
			//any slot in the queue?
			if (qtop != 0) {
				releaseTopThread();

				while (qtop && qtop->level == curLevel) {
					releaseTopThread();
				}
			}
		}
	}

	void releaseTopThread()
	{
		//pick top slot
		Slot *s = qtop;
		//set new level
		curLevel = s->level;
		//remove from the queue
		qtop = qtop->next;
		//set use count 
		useCount++;
		//mark slot signaled
		s->signaled = true;
		//wakeup thread
		s->notify->wakeUp();
	}

	atomicValue getLevel() const {
		return curLevel;
	}
protected:

	///the top and the bottom queue
	Slot *qtop,*qbot;
	///current lock level
	atomicValue curLevel;
	///count of owners at current levels
	natural useCount;
	///fast lock to protect internal state during the operation
	FastLock protectInternals;

};

class RWLockBase {
public:


	///Lock for reading
	void lockRead() {
		lvlock.lock(0);
	}

	///unlocks for reading
	void unlockRead() {
		lvlock.unlock();
	}

	///locks for write
	void lockWrite() {
		lvlock.lock(getCurThreadId());
	}

	///unlocks write
	void unlockWrite() {
		lvlock.unlock();
	}

	///try lock for reading
	bool tryLockRead() {
		return lvlock.tryLock(0);
	}

	bool tryLockWrite() {
		return lvlock.tryLock(getCurThreadId());

	}

	void unlock() {
		lvlock.unlock();
	}

	class WriteLock {
	public:
		void lock();
		void unlock();
		bool tryLock();
	};

	class ReadLock {
	public:
		void lock();
		void unlock();
		bool tryLock();
	};


protected:

	LevelLock lvlock;
};


///Very simple and fast Read/Write lock
/** while lockRead is issued, lock is requested to acquire resource for reading.
 * Reading is shared mode, there can be more threads, that are reading. But
 * once lock is locked for reading, any other lock for writing are blocked.
 *
 * Writing mode works similar as standard FastLock. Only one owner can own
 * lock locked for writing. This also block any attempts to lock for reading
 *
 *
 * This class can be split to two parts, where each part act as standalone lock.
 *
 * Both locks can be used with Synchronize class.
 *
 *
 *
 */
class RWLock: public RWLockBase,
				  public RWLockBase::WriteLock,
				  public RWLockBase::ReadLock {
public:

	///Cast object to this class to receive read lock with standard lock-interface
	typedef RWLockBase::ReadLock ReadLock;
	///Cast object to this class to receive write lock with standard lock-interface
	typedef RWLockBase::WriteLock WriteLock;

};


inline void RWLockBase::WriteLock::lock() {
	static_cast<RWLock *>(this)->lockWrite();
}

inline void RWLockBase::WriteLock::unlock() {
	static_cast<RWLock *>(this)->unlockWrite();
}

inline bool RWLockBase::WriteLock::tryLock() {
	return static_cast<RWLock *>(this)->tryLockWrite();
}

inline void RWLockBase::ReadLock::lock() {
	static_cast<RWLock *>(this)->lockRead();
}

inline void RWLockBase::ReadLock::unlock() {
	static_cast<RWLock *>(this)->unlockRead();
}

inline bool RWLockBase::ReadLock::tryLock() {
	return static_cast<RWLock *>(this)->tryLockRead();
}


}

#endif /* LIGHTSPEED_MT_FASTRWLOCK_H_ */



