#include "syncPt.h"
/*
 * rwlock.h
 *
 *  Created on: 14.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_RWLOCK_H_
#define LIGHTSPEED_MT_RWLOCK_H_

#include "syncPt.h"
#include "mutex.h"

namespace LightSpeed {


	class ShareExclusiveLock {
	public:

		class SharedSlot: public SyncPt::Slot {
			friend class ShareExclusiveLock;
		public:
			virtual void wakeUp(natural reason = 0) throw();

		protected:
			ShareExclusiveLock *owner;

		};

		class ExclusiveSlot: public SyncPt::Slot {
			friend class ShareExclusiveLock;
		public:
			virtual void wakeUp(natural reason = 0) throw();

		protected:
			ShareExclusiveLock *owner;

		};

		typedef SyncPt::Slot Slot;

		ShareExclusiveLock():readerCount(0),locked(0) {}

		void lockAsync(SharedSlot &slot);
		void lockAsync(ExclusiveSlot &slot);
		void lockCancel(Slot &slot);
		void unlock();

	protected:
		atomic readerCount;
		atomic locked;
		SyncPt syncPoint;
		FastLock lk;

		friend class SharedSlot;
		friend class ExclusiveSlot;

	};

#if 0
	///Readers-writers-lock
	/** Optimizes locking while the caller exactly know, if requests reading or writting operation.
	 * While reading operation is requested, guarded region can be read by many readers,
	 * but cannot be written. While writing operation is requested, guarded region can be written
	 * and read only by one thread at time.
	 *
	 * Class handles situations, when writing is requested and there are a  lot of
	 * readers. It causes waiting caller until all readers leaves the region. Any
	 * readers requesting the lock must wait in the queue.
	 *
	 ** @note lock is not recursive
	 */
	class ShareExclusiveLock {
	public:

		class SlotShared: public SyncPt::Slot {
		public:
			SlotShared(Mutex &toUnlock, atomic &rdcnt)
				:toUnlock(toUnlock),rdcnt(rdcnt) {}

			virtual void notify(natural reason) {
				lockInc(rdcnt);
				toUnlock.unlock();
				SyncPt::Slot::notify(reason);
			}
		protected:
			Mutex &toUnlock;
			atomic &rdcnt;
		};

		class SlotExclusive: public SyncPt::Slot, public ISleepingObject {
		public:
			SlotExclusive(Gate &gt, atomic &rdcnt)
				:gt(gt),rdcnt(rdcnt),gtslot(*this) {}

			virtual void notify(natural reason) {
				if (!gtslot) {
					gt.close();
					if (readAcquire(&rdcnt) == 0) gt.open();
					if (gtslot) gt.waitAsync(gtslot);
				} else {
					SyncPt::Slot::notify(reason);
				}
			}
			virtual void wakeUp(natural reason) {
				SyncPt::Slot::notify(reason);
			}
		protected:
			Gate &gt;
			atomic &rdcnt;
			StncPt::Slot &gtslot;
		};

		ShareExclusiveLock();

		bool lockAsync(SlotShared &sl) {
			if (flock.lockAsync(sl)) return true;
			else return false;
		}

		bool lockAync(SlotExclusive &sl) {
			if (flock.lockAsync(sl)) {
				sl.notify(0);
			}
		}

		bool lockShared(const Timeout &tm) {

			if (!flock.lock(tm)) return false;
			lockInc(rdcnt);
			flock.unlock();
			return true;

		}

		void unlockShared() {
			if (lockDec(rdcnt) == 0) {
				rls.open();
			}

		}

		bool lockExclusive(const Timeout &tm) {
			if (flock.lock(tm)) return false;
			if (readAcquire(&rdcnt) == 0) rls.open();
			if (!rls.wait(tm)) {
				flock.unlock();
				return false;
			}
			return true;
		}

		void unlockShared() {
			rls.close();
			flock.unlock();
		}

	protected:
		Mutex flock;
		Gate rls;
		atomic rdcnt;

	};
#endif
}  // namespace LightSpeed

#endif /* LIGHTSPEED_MT_RWLOCK_H_ */
