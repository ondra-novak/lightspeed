#pragma  once

#include "../exceptions/exception.h"

namespace LightSpeed {


	///Implements proxy lock object to handle locking two or more standalone locks
	/** To acquire more then one lock, you have to be very careful, especially
	  * when you cannot determine relationship between these lock. Which lock
	  * should be acquired first and which second?
	  *
	  * This class can acquire both locks safety, handing all possibilities
	  * to prevent unwanted deadlock. Also handles any exception thrown
	  * from any lock.
	  *
	  * Implementation will wait for acquiring the first lock, 
	  * and the it try to acquire second lock. Operation try is not
	  * blocking, so if this lock is acquired by another owner, it
	  * first releases first lock and repeating procedure with reversed order
	  * 
	  * This prevents keeping acquired one lock while waiting to other. This
	  * can effectively prevent any deadlock, when releasing of one lock
	  * depend on releasing of other lock. Note that this expects, that
	  * situation when both locks are un-acquired may happen.
	  * Otherwise, this implementation can also deadlock.(for example:
	  * you have thread which acquires first lock and then releases other lock,
	  * and this repeats swapping the locks).
	  */

	template<typename Lock1, typename Lock2>
	class LockPair {

	public:

		/// Constructs proxy lockPair.
		/**
		 * @param lk1 reference to first lock. Object keeps reference, don't
			dispose the lock before this object is destroyed
			* @param lk2 reference to second lock. Object keeps reference, don't
			dispose the lock before this object is destroyed
		 */
		LockPair(const Lock1 &lk1, const Lock2 &lk2):lk1(lk1),lk2(lk2) {}
		
		///Locks the  instance
        /**
		   Safety acquires both locks 
		   @exception any Re-trows exception thrown by any lock during process.
					  Before exception is rethrown, it releases any 
					  lock acquired during process before.

         */
        void lock() const;
        ///Unlocks the  instance
        /**
         * @exception any Re-throws exception thrown during unlock, but it
				finishes releasing before. So even unlock() of any lock
				thrown exception, function still treats the lock released.
				In double exception, only last exception is thrown
         */
        void unlock() const;

        ///Try locks the instance
        /**
			Acquires locks without blocking
         * @retval true locks acquired
		 * @retval false one of locks cannot be acquired. Object remains unlocked
			 @exception any Re-trows exception thrown by any lock during process.
							 Before exception is rethrown, it releases any 
							lock acquired during process before.
         */
        bool tryLock() const;

		///Retrieves current lk1 value
		/** 
		*	@return current lk1 value
		*/
		const Lock1 & getLock1() const { return lk1; }

		///Retrieves current lk2 value
		/** 
		*	@return current lk2 value
		*/
		const Lock2 & getLock2() const { return lk2; }

		///Locks the instance specifying timeout to wait
		/**
		 * @param timeout Specifies timeout
		 * @retval true locked
		   @retval false timeout
		   @exception any Re-trows exception thrown by any lock during process.
					  	 Before exception is rethrown, it releases any 
						 lock acquired during process before.
		 */
		template<typename TimeoutSpec>
		bool lock(const TimeoutSpec &timeout) const;

	protected:

		const Lock1 &lk1;
		const Lock2 &lk2;

	};

	template<typename Lock1, typename Lock2>
	bool LightSpeed::LockPair<Lock1, Lock2>::tryLock() const
	{
		if (lk1.tryLock()) {
			try {
				if (lk2.tryLock())
					return true;
			} catch (...) {
				lk1.unlock();
				Exception::rethrow(THISLOCATION);
			}
			lk1.unlock();			
		}
		return false;
	}
	template<typename Lock1, typename Lock2>
	void LightSpeed::LockPair<Lock1, Lock2>::unlock() const
	{
		try {
			lk1.unlock();
		} catch (...) {
			lk2.unlock();
			Exception::rethrow(THISLOCATION);
		}
		lk2.unlock();
	}
	template<typename Lock1, typename Lock2>
	void LightSpeed::LockPair<Lock1, Lock2>::lock() const
	{
		do {
			lk1.lock();
			try {
				if (lk2.tryLock())  return;
			} catch (...) {
				lk1.unlock();
				Exception::rethrow(THISLOCATION);
			}
			lk1.unlock();

			lk2.lock();
			try {
				if (lk1.tryLock())  return;
			} catch (...) {
				lk2.unlock();
				Exception::rethrow(THISLOCATION);
			}
			lk2.unlock();
		} while (true);
	}

	template<typename Lock1, typename Lock2>
	template<typename TimeoutSpec>
	bool LightSpeed::LockPair<Lock1, Lock2>::lock( const TimeoutSpec &timeout ) const
	{
		do {
			if (!lk1.lock(timeout)) return false;
			try {
				if (lk2.tryLock())  return true;
			} catch (...) {
				lk1.unlock();
				Exception::rethrow(THISLOCATION);
			}
			lk1.unlock();

			if (!lk2.lock(timeout)) return false;
			try {
				if (lk1.tryLock())  return true;
			} catch (...) {
				lk2.unlock();
				Exception::rethrow(THISLOCATION);
			}
			lk2.unlock();
		} while (true);
	}
}
