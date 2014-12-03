#pragma once
#include "syncPt.h"
#include "mutex.h"
#include "sleepingobject.h"
#include "..\base\containers\optional.h"
#include "..\base\exceptions\stdexception.h"
#include "exceptions\timeoutException.h"

namespace LightSpeed {

	///Allows to share single value between threads at given time
	/** 
	 * If you need to copy variable from one thread to another at defined point. 
	 *
	 * SharePt is similar to SyncPt, in exception that you can carry a value from one thread to another.
	 *
	 * Thread that have a variable to copy issues share() and specifies value and count of threads, that will
	 *  receive the value. Function share() blocks until variable is copied to the another thread, which 
	 * calls read() function. Function read() can also block when there is no sharing available. It will
	 * unblock once the other thread calls share()
	 *
	 * There are also asynchronous version of share() - shareAsync() and read() - readAsync()
	 *
	 * @tparam T type of variable which is shared
	 *
	 * @note Sharing in this terminology means that variable is actually copied from one thread to another. It
	 * doesn't require any extra space to store variable during synchronization. Also note that variable can be
	 * copied twice between shares so speed of copying can be critical for good performance
	 */
	template<typename T>
	class SharePt: public SyncPt {
	public:

		SharePt():countShares(0) {}

		///Shares object synchronously
		/** Stops processing code until object specified by obj
		   is transfered into other thread. 
		   @param obj object to transfer
		   @param count count of thread can read this object
		   @param tm timeout to wait

		   @retval true successfully shared
		   @retval false error - object is in use (another thread is sharing)

		   */
		bool share(const T &obj, natural count,  const Timeout tm = naturalNull);

		///Shares object asynchronously
		/** Asynchronous sharing  - shares object without need to wait

		   @param obj object to share. Must exist during sharing (you needn't to destroy it during sharing)
		   @param count count of sharing. This value should be 1 or above
		   @param completion reference to an object that receives completion notification

		   @retval true successfully setup
		   @retval false object is in use (another thread is sharing)

		 */		
		
		bool shareAsync(const T &obj, natural count, ISleepingObject &completion);


		///Cancels asynchronous sharing
		/** You have to cancel asynchronous sharing if you need to stop sharing prematurely. Canceling share
		    doesnt trigger completion. You have to cancel only asynchronous sharing

			@param completion reference to completion object to check, whether this request is still valid (it could
			   become invalid due race condition)
			@retval true canceled
			@retval false not-canceled probably not in shared state or another thread already started new sharing
	    */
		bool cancelShareAsync(ISleepingObject &completion);

		///Use this class to build read slosts for asynchronous reading
		typedef Slot ReadSlot;


		///Reads value of shared variable.
		/**
		  Function waits for sharing and reads its value. If sharing is already in progress, function immediatelly returns 
		  value. Otherwise, it blocks until sharing is available.

		  @param tm timeout 
		  @return function returns read value. 
		  @exception TimeoutException timeout ellapsed
		  @exception any Exception during copying object
		 */

		   
		T read(const Timeout tm = naturalNull);
		
		///Reads value of shared variable asynchronously
		/**
		  Function registers slot on sync point and then checks, whether there is sharing in progress. In
		  that case, immediately switch slot into signaling state calling its wakeUp event. Value is stored
		  into payload and it is valid only during wakeUp() call, even if address remains. Do not access
		  data specified by "payload" address outside of wakeUp() call

		  For convenience, you can use StoreSlot class to store result without need to implementing relative hard way
		  to copy value during wakeUp. Note that you shouldn't touch instance of slot while it is registered for asynchronous
		  reading

		  To cancel asynchronous operation, call cancelSlot() or remove() function. Always use that function, if you
		  not sure, that operation completed.

		  @param slot reference to reading slot (ReadSlot or StoreSlor)
		  
		*/
		  
		void readAsync(ReadSlot &slot);

		

		class StoreSlot: public ReadSlot, public Optional<T> {
		public:
			StoreSlot() {}
			StoreSlot(IWaitingObject &nxt):ReadSlot(nxt) {}

			virtual void notify(natural reason = 0) {
				try {
					this->init(*(T *)payload);
				} catch (Exception &e) {
					this->e = e.clone();
				} catch (std::exception &e) {
					this->e = new StdException(THISLOCATION,e);
				} catch (...) {
					this->e = new UnknownException(THISLOCATION);
				}
				ReadSlot::notify(reason);
			}

			PException e;
		};	

	protected:

		using SyncPt::add;

		void doShare();

		const T *object;
		atomic countShares;
		ISleepingObject *completion;
	};


	template<typename T>
	bool LightSpeed::SharePt<T>::share( const T &obj, natural count, const Timeout tm /*= naturalNull*/ )
	{
		Notifier completion;
		if (!shareAsync(obj,count,completion)) return false;
		completion.wait(tm);
		cancelShareAsync(completion);
		return true;
	}


	template<typename T>
	T LightSpeed::SharePt<T>::read( const Timeout tm /*= naturalNull*/ )
	{		

		Synchronized<FastLock> _(this->queueLock);
		if (countShares > 0) {
			countShares--;
			if (countShares == 0) this->completion->wakeUp();
			return *object;
		} else {
			StoreSlot ntfcb;
			this->add_lk(ntfcb);
			SyncReleased<FastLock> _(this->queueLock);
			wait(ntfcb,tm);
			this->remove(ntfcb);
			if (!ntfcb) throw TimeoutException(THISLOCATION);
			if (ntfcb.e != nil) ntfcb.e->throwAgain(THISLOCATION);
			return *ntfcb;
		}

	}

	template<typename T>
	bool LightSpeed::SharePt<T>::cancelShareAsync( ISleepingObject &completion )
	{
		Synchronized<FastLock> _(this->queueLock);
		if (this->completion == &completion) {
			countShares = 0;
			return true;
		} else {
			return false;
		}
	}

	template<typename T>
	void LightSpeed::SharePt<T>::doShare()
	{
		
		if (countShares) {
			Slot *slt = this->popSlot_lk();
			while (slt) {
				ReadSlot *rdslot = static_cast<ReadSlot *>(slt);
				rdslot->payload = const_cast<T *>(object);
				this->notifySlot_lk(rdslot);
				if (--countShares)
					slt = this->popSlot_lk();
				else 
					break;
			}
			if (countShares == 0) {
				completion->wakeUp(0);
			}
		}
	}

	template<typename T>
	void LightSpeed::SharePt<T>::readAsync( ReadSlot &slot )
	{
		this->add(slot);
		Synchronized<FastLock> _(this->queueLock);
		doShare();
	}

	template<typename T>
	bool LightSpeed::SharePt<T>::shareAsync( const T &obj, natural count, ISleepingObject &completion )
	{	
		Synchronized<FastLock> _(this->queueLock);
		if (countShares) return false;
		object = &obj;
		this->completion = &completion;
		this->countShares = count;

		doShare();
		return true;
	}


}