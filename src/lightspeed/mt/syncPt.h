/*
 * isyncpoint.h
 *
 *  Created on: 20.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_SYNCPT_H_
#define LIGHTSPEED_MT_SYNCPT_H_

#include "sleepingobject.h"
#include "notifier.h"
#include "atomic_type.h"
#include "slist.h"
#include "fastlock.h"


namespace LightSpeed {

	class Timeout;


	///Basic synchronization between threads
	/** This object is main part of all high-level synchronization objects.
	 * It contains the container of threads, which waiting for notification
	 * from another thread. When other thread wants to notify waiting threads,
	 * it can notify first or all objects in the container.
	 *
	 * Thread that wish to wait creates SyncPt::Slot object which is registered
	 * on the SyncPt and then it can be signaled by some other thread. The thread can
	 * assign itself to the SyncPt::Slot as ISleepingObject and then it is able
	 * to sleep until notification is caught causing thread wakening up.
	 *
	 * This allows to the thread to wait on multiple sync.points while first
	 * notification wakes the waiting thread and it is able to find out, which
	 * notification was it
	 *
	 * If thread wish to wait only one sync.point at time, it can use
	 * method wait() without need to creating SyncSlot.
	 *
	 * @note destroying SyncPt with waiting slots will not notify these slots causing
	 * a possible deadlock.
	 *
	 *
	 * Best practise for asynchronous waiting
	 *
	 * - check condition, if passed, no waiting is needed
	 *
	 * - create slot
	 *
	 * - register slot
	 *
	 * - check condition, if passed, remove slot, continue
	 *
	 * - wait, if wait successed, continue
	 *
	 * - if failed, remove slot - if failed, wait succesed, continue
	 *
	 * - if successed, throw timeout
	 *
	 *
	 */
	class SyncPt {
	public:

		///Sync slot
		/** Inherits Notifier but can be used only with SyncPt. It is constructed not-signaled */
		class Slot: private Notifier {
		public:
			///Creates slot which notifies current thread
			Slot():payload(0),next(0),charged(false) {}
			///Creates slot which notifies specified waiting object
			Slot(ISleepingObject &forward):Notifier(forward),payload(0),next(0),charged(false) {}

			virtual ~Slot()  {}

			///tests variable, whether is signaled
			operator bool() const {return Notifier::operator bool();}
			///tests variable, whether is not-signaled
			bool operator !() const {return Notifier::operator !();}

			///Retrieves id of thread associated with this object
			/**
			 * @return associated thread.
			 *
			 * @note function can return valid result only if previous setThreadId has
			 * been called. Otherwise, result is undefined. Function is used
			 * by several locks where ownership is bound to the thread.
			 */
			atomicValue getThreadId() const {return threadId;}
			///Associates this object with the thread using atomic thread identifer
			/**
			 * @param thrId id of thread associated with this object
			 */
			void setThreadId(atomic thrId) {threadId = thrId;}

			bool isSignaled() const {return Notifier::operator bool();}

			virtual void notify(natural reason = 0) {
				Notifier::wakeUp(reason);
			}

			union {
				atomic threadId; ///<user defined for threadId - not set by constructor
				void *payload;	///<user defined for payload - anything can be stored here on notify
				natural value;  ///<user defined for value - anything can be stored here on notify
			};

			///resets slots and allows it to be resued			
			/**
			 @retval true slot has been reset. Object that inherits this class can reset its internal state
			 @retval false slot has not been reset (not signaled). Object that inherits this class should not
			  touch internal state, because it can be accessed form another thread */
			bool reset() {
				if (isSignaled()) {
					Notifier::reset();
					return true;
				} else {
					return false;
				}
			}

			const Notifier &getNotifier() const {return *this;}
		protected:
			friend class SyncPt;
			friend class SList<Slot>;
			Slot *next;
			bool charged;

		};

/*
		///Helps to build custom slots with autocancel ability
		template<typename T>
		class SlotT: public Slot {
		public:
			///create slot and link with owner
			SlotT(T &owner):owner(owner) {}
			///create slot and link with owner
			SlotT(ISleepingObject &forward):Slot(forward) {}
			///destroy slot and notify owner about it
			~SlotT() {
				if (charged && !isSignaled()) owner.cancelSlot(*this);
			}
		protected:
			T &owner;
		};
*/
		enum Order {
			queue, stack
		};

		SyncPt();
		SyncPt(Order order);
		~SyncPt();

		enum WaitInterruptMode {

			///uninterruptible
			/** waiting cannot be interrupted before event on timeout reached */
			uninterruptible,
			///interrupt waiting on set exit flag
			/** waiting will be interrupted, when Thread::setExitFlag is called,
			 *
			 * @note you cannot continue wait, because exit flag remains active
			 *  */
			interruptOnExit,
			///interrupt wait on another event
			/**
			 * Interrupts waiting on any event caught by wakeUp() method.
			 * You have a chance to check, which event arrived and continue
			 * in waiting. You can continue in waiting in case that
			 * interruption is caused by setExitFlag(). In this case
			 * flag itself is not checked, function only wakeups the
			 * thread.
			 */
			interruptable
		};


		///Wait on sync.point without need to create SyncSlotBase
		/**
		 * Simple waiting.
		 * @param sl reference to slot charged into SyncPt
		 * @param tm specifies timeout
		 * @param mode specifies, how waiting can be interrupted
		 * @retval true wait success
		 * @retval false timeout
		 */
		static bool wait(Slot &sl,const Timeout &tm, WaitInterruptMode mode = uninterruptible);


		///Registers slot to the sync.point
		/**
		 * @param slot slot to register
		 * @retval true success, registered
		 * @retval false slot already registered
		 *
		 * Once slot is signaled, it is removed from pool.
		 */
		bool add(Slot &slot);



		///Removes slot from the sync.point
		/**
		 * @param slot slot to remove.
		 *
		 *  @retval true slot removed
		 *  @retval false slot was not found. CHECK WHETHER IS IT SIGNALED!
		 *
		 *  @code
		 *  if (!syncpt.wait(slot,...) && remove(slot)) {
		 *     throw Timeout();
		 *  } else if (slot.isSignaled()) {
		 *  	//signaled
		 *  } else {
		 *  	// error, should not reach here
		 *  }
		 *
		 *  @endcode
		 *
		 *  @code
		 *  if (!remove(slot) && slot.isSignaled()) {
		 *     //signaled
		 *  }
		 *
		 *
		 */
		bool remove(Slot &slot);




		///Notifies one slot
		/**
		 * notifies one slot, doesn't modify payload
		 *
		 * @retval true notified
		 * @retval false empty queue
		 *
		 * @note  Note that notifications
		 *  and cancellation are mutual exclusive operations and may block each other. Do not
		 *  call remove() in notification event (causing deadlock)
		 */
		bool notifyOne();

		///Notifies all slots
		/**
		 * notifies all slots, doesn't modify payload
		 * @retval true notified
		 * @retval false empty queue
		 *
		 * @note notification is treat as atomic. You can add new items to the queue during notification.
		 *  They will not notified. This is required to implement barriers. Note that notifications
		 *  and cancellation are mutual exclusive operations and may block each other. Do not
		 *  call remove() in notification event (causing deadlock)
		 *
		 */
		bool notifyAll();


		bool empty() const {return curQueue == 0;}

		void cancelSlot(Slot &sl) {remove(sl);}

	protected:

		//acquire lock to call following two functions
		FastLock queueLock;

		//call under lock!
		Slot *popSlot_lk();

		//call under lock!
		void notifySlot_lk(Slot *slot);

		void add_lk( Slot &slot );


	private:

		Slot * volatile curQueue;
		Order order;


		SyncPt(const SyncPt &pt);
		SyncPt &operator=(const SyncPt &pt);


	};

	///Synchronize point with ability to transfer value during notification
	/**
	 * Value is transfered synchronously, so neither source nor target
	 * thread can affect value. Target thread will receive notification
	 * at same time as the value.
	 *
	 * @tparam type of value
	 */
	template<typename T>
	class SyncPtT: public SyncPt {
	public:
		SyncPtT() {}
		SyncPtT(Order order):SyncPt(order) {}


		///always use this slot
		class Slot: public SyncPt::Slot {
		public:
			///Constructs slot and initializes the variable
			Slot(T variableToSet):value(variableToSet) {}
			///Constructs slot and uses default value for the variable
			Slot() {}

			///value is stored here
			T value;
		};

		///Adds slot to receive notification
		/**
		 * @param slot slot to add
		 * @retval true success
		 * @retval false
		 */
		bool add(Slot &slot) {return SyncPt::add(slot);}

		///Notifies first slot in the queue and transfers the value
		/**
		 * @param val value to copy
		 * @retval true notified
		 * @retval false queue were empty
		 */
		bool notifyOne(const T &val);
		///Notifies all slots in the queue and transfers the value to them
		/**
		 * @param val value to copy
		 * @retval true notified
		 * @retval false queue were empty
		 */
		bool notifyAll(const T &val);
	protected:
		//call under lock!
		void notifySlot_lk(Slot *slot, const T &val);

	};

	template<typename T>
	inline bool SyncPtT<T>::notifyOne(const T& val) {
		Synchronized<FastLock> _(queueLock);
		Slot *p = static_cast<Slot *>(popSlot_lk());
		if (p == 0) return false;
		notifySlot_lk(p,val);
		return true;
	}

	template<typename T>
	inline bool SyncPtT<T>::notifyAll(const T& val) {
		Synchronized<FastLock> _(queueLock);
		bool succ = false;
		Slot *p = static_cast<Slot *>(popSlot_lk());
		while (p) {
			notiftSlot_lk(p,val);
			succ = true;
			p = static_cast<Slot *>(popSlot_lk());
		}
		return succ;
	}

	template<typename T>
	inline void SyncPtT<T>::notifySlot_lk(Slot* slot, const T& val) {
		slot->value = val;
		slot->notify();
	}

}


#endif /* LIGHTSPEED_MT_SYNCPT_H_ */
