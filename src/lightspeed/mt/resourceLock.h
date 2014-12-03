/*
 * resourceLock.h
 *
 *  Created on: 10.5.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_RESOURCELOCK_H_
#define LIGHTSPEED_MT_RESOURCELOCK_H_
#include "exceptions/timeoutException.h"
#include "syncPt.h"
#include "../base/containers/map.h"
#include "shareExclusiveLock.h"

namespace LightSpeed {


	///Allows to lock single resource from container of resources
	/**
	 * Class holds locks referenced by resource's ID, where ID is anything, which
	 * can be used as key into the table. Lock is associated with the ID. You
	 * can make readLock or writeLock similary to ShareExclusiveLock.
	 *
	 * ResourceLock extends ShareExclusiveLock. Itself can be used in Shared-Exclusive mode, while
	 * lock itself is shared-locked, when there is any resource locked by any lock mode.
	 * ResourceLock never use exclusive lock, but when function writeLock is called,
	 * any try of use resource lock is blocked until exclusive lock is released
	 *
	 * This allows to use ResourceLock to guard items in the container and conteiner
	 * itself. You can exclusive lock whole conteiner and prevent to any other
	 * thread to obtain any item lock, until container is stable state.
	 *
	 *
	 * @tparam ResID type of resource id
	 * @tparam Operator to compare resource ids
	 * @tparam Alloc allocator used to allocate internals. Don't need to be thread safe
	 */
	template<typename ResID, typename Cmp = std::less<ResID>, typename Alloc = ClusterFactory<> >
	class ResourceLock: public ShareExclusiveLock {
	public:
		typedef ResID ResourceID;

		///Creates resource lock instance
		ResourceLock() {}
		///Creates resource lock instance and initializes operators
		/**
		 * @param cmp instance of compare operator
		 * @param alloc instance of allocator
		 */
		ResourceLock(const Cmp &cmp,const Alloc alloc):lockMap(cmp,alloc) {}

		using ShareExclusiveLock::lockShared;
		using ShareExclusiveLock::lockExclusive;
		using ShareExclusiveLock::unlock;

		///Makes read (shared) lock
		/**
		 * @param resid identifier of the resource
		 * @param tm timeout to wait for lock, nil for infinite, 0 for no wait
		 * @param md how to handle interruptions
		 * @retval true success
		 * @retval false timeouted or interrupted
		 */
		bool lockShared(const ResID &resid, const Timeout &tm = nil,
				SyncPt::WaitInterruptMode md = SyncPt::uninterruptable);


		///Makes write (exclusive) lock
		/**
		 * @param resid identifier of the resource
		 * @param tm timeout to wait for lock, nil for infinite, 0 for no wait
		 * @param md how to handle interruptions
		 * @retval true success
		 * @retval false timeouted or interrupted
		 */
		bool lockExclusive(const ResID &resid, const Timeout &tm = nil,
				SyncPt::WaitInterruptMode md = SyncPt::uninterruptable);

		///Unlocks the resource
		void unlock(const ResID &resid);




	protected:

		typedef Map<ResID, ShareExclusiveLock, Cmp, Alloc> LockMap;
		typedef ShareExclusiveLock::Lock Sync;
		typedef LightSpeed::SyncReleased<SyncPt::LockImpl> RSync;
		LockMap lockMap;

		template<typename S>
		bool readWriteLock(const ResID &resid, const Timeout &tm = nil,
				SyncPt::WaitInterruptMode md = SyncPt::uninterruptable);

	};

	///Provides automatic shared locking in the function scope
	template<typename T>
	class ResourceSyncR {
	public:
		typedef typename T::ResourceID ResID;
		ResourceSyncR(T &lock, const ResID &resId, const Timeout &tm = nil,
				SyncPt::WaitInterruptMode md = SyncPt::uninterruptable)
			:lock(lock),resId(resId)
		{
			if (!lock.lockShared(resId,tm,md))
				throw TimeoutException(THISLOCATION);
		}

		~ResourceSyncR() {
			lock.unlock(resId);
		}

	protected:
		T &lock;
		ResID resId;
	};

	///Provides automatic exclusive locking in the function scope
	template<typename T>
	class ResourceSyncW {
	public:
		typedef typename T::ResourceID ResID;
		ResourceSyncW(T &lock, const ResID &resId, const Timeout &tm = nil,
				SyncPt::WaitInterruptMode md = SyncPt::uninterruptable)
			:lock(lock),resId(resId)
		{
			if (!lock.lockExclusive(resId,tm,md))
				throw TimeoutException(THISLOCATION);
		}

		~ResourceSyncW() {
			lock.unlock(resId);
		}

	protected:
		T &lock;
		ResID resId;
	};


template<typename ResID, typename Cmp, typename Alloc>
bool LightSpeed::ResourceLock<ResID,Cmp,Alloc>::lockShared(const ResID & resid,
		const Timeout & tm, SyncPt::WaitInterruptMode md) {

	return readWriteLock<SyncSlotShared>(resid,tm,md);
}

template<typename ResID, typename Cmp, typename Alloc>
template<typename S>
bool LightSpeed::ResourceLock<ResID,Cmp,Alloc>::readWriteLock(const ResID & resid,
		const Timeout & tm, SyncPt::WaitInterruptMode md)

{
	bool retry;
	do {
		Sync _(inrlk);
		if (exclusiveOwner != 0) {
			RSync _(inrlk);
			if (lockShared(tm,md) == false) return false;
			retry = true;
		} else {
			rdCount++;
			retry = false;
		}
	} while (retry);
	ShareExclusiveLock &lk = lockMap(resid);
	S slot(lk);
	if (slot) return true;
	{
		RSync _(inrlk);
		slot.wait(tm,md);
	}
	if (slot)
		return true;
	else {
		rdCount--;
		return false;
	}

}



template<typename ResID, typename Cmp, typename Alloc>
bool LightSpeed::ResourceLock<ResID,Cmp,Alloc>::lockExclusive(const ResID & resid,
		const Timeout & tm, SyncPt::WaitInterruptMode md)
{
	return readWriteLock<SyncSlotExclusive>(resid,tm,md);
}



template<typename ResID, typename Cmp, typename Alloc>
void LightSpeed::ResourceLock<ResID,Cmp,Alloc>::unlock(const ResID & resid)
{
	Sync _(SyncPt::inrlk);
	ShareExclusiveLock &lk = lockMap(resid);
	lk.unlock();
	if (!lk.isLocked()) lockMap.erase(resid);
	if (rdCount > 0) --rdCount;
}

}



#endif /* LIGHTSPEED_MT_RESOURCELOCK_H_ */
