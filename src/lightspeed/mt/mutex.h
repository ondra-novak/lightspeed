/*
 * mutex.h
 *
 *  Created on: 17.5.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_MUTEX_H_
#define LIGHTSPEED_MT_MUTEX_H_
#include "syncPt.h"
#include "atomic_type.h"
#include "threadId.h"
#include "timeout.h"

namespace LightSpeed {

class Mutex {
	class SetOwner;
	friend class SetOwner;
public:

	Mutex():owner(0),recursion(0) {};

	bool lock(const Timeout &tm);

	void lock();


	bool tryLock();

	void unlock();



	///Lock key asynchronous
	/** You have to create Slot which will be signaled when lock is achieved
	 *
	 * @param slot reference to object SyncPt::Slot to be registered in lock's
	 * queue, It must be valid until lock is achieved
	 * @retval true lock achieved without need to wait
	 * @retval false lock is locked, you need wait
	 */
	bool lockAsync(SyncPt::Slot &slot);

	///cancels waiting on asynchronous lock
	/**
	 * You need call this function to cancel waiting. Function removes
	 * slot from queue
	 * @param sl slot to remove
	 *
	 * Because slot can become signaled, function also unlock already achieved
	 * lock.
	 */
	void cancelAsync(SyncPt::Slot &sl);

	///Returns mutex's recursive counter
	/** Can be useful to limit count of recursion in exclusive code */
	natural getTurns() const {return recursion;}

	///Returns ID of the owner
	atomicValue getOwner() const {return owner;}

	///Changes ownership of mutex
	/**Gives ownership to the another thread. You have to use proper
	 * synchronization to notify other thread about gained ownership.
	 * You can use function to give ownership to newly created thread
	 * for example.
	 *
	 * @param newOwner ID of new owner
	 * @retval true success
	 * @retval false can't give ownership while mutex is not owned.
	 */
	bool setOwner(atomic newOwner);

protected:

	atomic owner;
	SyncPt waitPt;
	atomic recursion;
};



}




#endif /* LIGHTSPEED_MT_MUTEX_H_ */
