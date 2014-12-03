/*
 * spinlock.h
 *
 *  Created on: 14.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_SPINLOCK_H_
#define LIGHTSPEED_MT_SPINLOCK_H_

#include "atomic.h"
#include "threadId.h"

namespace LightSpeed {


	///Implements spin-lock
	/**Spin-lock is lock that guarantees exclusive access to the
		resource similar to mutex. There is difference between spin-lock
		and mutex. Spin-lock si very small lock which implements active
		waiting in cycle. During waiting, ownership is repeatedly checked
		and once ownership is released, lock takes the ownership and returns

		Active waiting in the cycle is the best for very small piece of
		code that needs to be done exclusive. It is very fast on multiprocessor
		environment where is chance, that lock will be released very soon.

		In single-processor environment the spinLock can be slower them mutex
		especially when thread which has ownership has lower priority than
		waiting thread. Scheduler may not give to the lock's owner chance
		to run and release the lock soon.

		@see SpinLockRecursive, CriticalSection
	*/
	class SpinLock {
	public:

		///construct spinlock
		SpinLock():owner(0) {}

		///tries to take ownership without waiting
		/**
		 * @retval true success, we have ownership
		 * @retval false failed, cannot get ownership
		 *
		 * @note Try to take ownership of owned lock is always successful. But
		 *   this lock doesn't support recursion.
		 *
		 */
		bool tryLock() {
			atomic id = ThreadId::current().asAtomic();
			atomic chk = lockCompareExchange(owner,0,id);
			return chk == 0 || chk == id;
		}

		///Unlocks the spin-lock
		void unlock() {
			atomic id = ThreadId::current().asAtomic();
			lockCompareExchange(owner,id,0);
		}

		///Locks the spin-lock
		/** Function will block, if lock is owned by other thread
		 *
		 */
		void lock() {
			bool rep;
			do {rep = tryLock();} while (rep == false);
		}

		///Locks the spin-lock
		/** Function will block, if lock is owned by other thread
		 * @param spinCount maximum of spin counts after function fails
		 * @retval true success
		 * @retval failed to take ownership after spin counts
		 *
		 */
		bool tryLock(natural spinCount) {
			for (natural i = 0; i < spinCount; i++)
				if (tryLock()) return true;
			return false;
		}

		///Check, whether lock is owned by current thread
		/**
		 * @retval true lock is owned by current thread
		 * @return false lock is not owned, or owned by another thread
		 */
		bool isOwned() const {
			return readAcquire(&owner) == ThreadId::current().asAtomic();
		}

		///Check, whether lock is owned by the thread
		/**
		 * @retval id Id of thread to check
		 * @retval true lock is owned by the thread
		 * @return false lock is not owned, or owned by another thread
		 */
		bool isOwned(const ThreadId &id) const {
			return readAcquire(&owner) == id.asAtomic();
		}

		///Checks, whether lock is unlocked
		/**
		 * @retval true lock is not locked
		 * @retval false lock is locked
		 * @note function cannot guarantee, that state will not changed
		 */
		bool isUnlocked() const {
			return readAcquire(&owner) == 0;
		}

		///gives ownership to the another thread
		/**
		 * Function is useful to give ownership of object to the newly
		 * created thread without need to unlock the lock (and cause
		 * that third thread successfully takes ownership
		 *
		 * @param to ID of thread that will receive ownership
		 * @retval true success
		 * @retval false fail, cannot give ownership of object, which doesn't own.
		 */
		bool give(const ThreadId &to) {
			if (isOwned()) {
				writeRelease(&owner,to.asAtomic());

				return true;
			} else {
				return false;
			}
		}


		///steals ownership from the thread to me
		/**Function is useful to unlock object owned by thread that is
		 * no longer exist
		 *
		 * @param from id of thread which owns to object
		 * @retval true success. After return, current thread owns the object
		 * and it have to unlock it when it no longer needed
		 * @retval false failed. Object is not probably owned by specified thread
		 */
		bool steal(const ThreadId &from) {
			if (isOwned(from)) {
				writeRelease(&owner,ThreadId::current().asAtomic());
				return true;
			} else {
				return false;
			}

		}

		///Retrieves current owner
		ThreadId getOwner() const {
			return ThreadId(readAcquire(&owner));
		}

	protected:

		atomic owner;

	private:
		void operator=(const SpinLock &other);
	};


	///Spin lock with recursive counter
	/**
	 * Handles recursive lockings. Object is unlocked when count of unlocks
	 * is equal to count of locks.
	 */
	class SpinLockRecursive: public SpinLock {
	public:

		SpinLockRecursive():recursiveCount(0) {}

		bool tryLock() {
			atomic id = ThreadId::current().asAtomic();
			atomic chk = lockCompareExchange(owner,0,id);
			if (chk == 0) {
				recursiveCount=1;
				return true;
			} else if (chk == id) {
				recursiveCount++;
				return true;
			} else {
				return false;
			}
		}

		void unlock() {
			atomic id = ThreadId::current().asAtomic();
			if (readAcquire(&owner) == id) {
				if (--recursiveCount == 0)
					writeRelease(&owner,0);
			}
		}

		void lock() {
			bool rep;
			do {rep = tryLock();} while (rep == false);
		}

		bool tryLock(natural spinCount) {
			for (natural i = 0; i < spinCount; i++)
				if (tryLock()) return true;
			return false;
		}

	protected:
		natural recursiveCount;
	};

}  // namespace LightSpeed

#endif /* LIGHTSPEED_MT_SPINLOCK_H_ */
