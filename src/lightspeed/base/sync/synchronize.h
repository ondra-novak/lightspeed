#ifndef LIGHTSPEED_BASE_SYNCHRONIZED_H_
#define LIGHTSPEED_BASE_SYNCHRONIZED_H_

#include "../meta/emptyClass.h"
#include "../exceptions/genexcept.h"



namespace LightSpeed {

	enum TryLockEnum {
		tryLock
	};

	extern LIGHTSPEED_EXPORT const char *Synchronized_TryLockFailedMsg;
	typedef GenException<Synchronized_TryLockFailedMsg> Synchronized_TryLockFailed;

    template<typename T, typename Spec = Empty>
    class Synchronized;
    template<typename T, typename Spec = Empty>
    class SyncReleased;
	///Provides basic synchronization.
	/** instance of this class holds a lock until destruction. In most of times,
	you will use this class to synchronize scope from place where instance
	is constructed to the end of scope
	*/
	template<typename T>
	class Synchronized<T,Empty> {
		 T &lk;
	public:


		///Constructs the instance and locks the specified loc 
		/**
		  @param lk Lock used to synchronization
		  @throws any exception defined by lock class in case, that lock cannot
		  be acquired
		  */
		Synchronized( T &lk):lk(lk) {
		    lk.lock();
		}

		Synchronized( T &lk, TryLockEnum):lk(lk) {
			if (lk.tryLock() == false) throw Synchronized_TryLockFailed(THISLOCATION);
		}

		///Releases the lock
		~Synchronized() {
		    lk.unlock();
		}
	
		Synchronized(const Synchronized &other):lk(other.lk) {
		    lk.lockR();
		}
	private:
		void operator=(const Synchronized &other);

	};
	
	///Reversed synchronize
	/** It unlock the lock inside of scope
	 */
	template<typename T>
	class SyncReleased<T,Empty> {
		 T &lk;
	public:
		///Constructs the instance and locks the specified loc
		/**
		  @param lk Lock used to synchronization
		  @throws any exception defined by lock class in case, that lock cannot
		  be acquired
		  */
		SyncReleased( T &lk):lk(lk) {
		    lk.unlock();
		}
		///Releases the lock
		~SyncReleased() {
		    lk.lock();
		}

		SyncReleased(const SyncReleased &other);
	private:
		void operator=(const SyncReleased &other);
	};


	template<typename T, typename Spec>
	class Synchronized {
		 T &lk;
		 Spec spec;
	public:


		///Constructs the instance and locks the specified loc
		/**
		  @param lk Lock used to synchronization
		  @throws any exception defined by lock class in case, that lock cannot
		  be acquired
		  */
		Synchronized( T &lk, const Spec &spec):lk(lk),spec(spec) {
		    lk.lock(spec);
		}

		Synchronized(const Synchronized &other):lk(other.lk),spec(other.spec) {
			lk.lockR(spec);
		}

		///Releases the lock
		~Synchronized() {
		    lk.unlock(spec);
		}

	private:
		void operator=(const Synchronized &other);

	};

	///Reversed synchronize
	/** It unlock the lock inside of scope
	 */
	template<typename T,typename Spec>
	class SyncReleased {
		 T &lk;
		 Spec spec;
	public:
		///Constructs the instance and locks the specified loc
		/**
		  @param lk Lock used to synchronization
		  @throws any exception defined by lock class in case, that lock cannot
		  be acquired
		  */
		SyncReleased( T &lk, const Spec &spec):lk(lk),spec(spec) {
		    lk.unlock(spec);
		}
		///Releases the lock
		~SyncReleased() {
		    lk.lock(spec);
		}

		SyncReleased(const SyncReleased &other);
	private:
		void operator=(const SyncReleased &other);
	};

}

#endif /*SYNCHRONIZED_H_*/
