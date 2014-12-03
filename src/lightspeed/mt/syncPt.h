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

		enum Order {
			queue, stack
		};

		SyncPt();
		SyncPt(Order order);

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
		 * @param slot slot to remove. If slot has not been registered, function
		 * 	does nothing. If slot is waiting, but not at this sync point,
		 *  function doesn't change state of slot from waiting to clean.
		 *
		 *  You can use SyncSlotBase::getState() to test, whether slot has been
		 *  removed.
		 *
		 *  @note in connection with object based on SyncPt, removing slot
		 *  from the SyncPt object is interpreted as give up of waiting. During
		 *  removin, slot may be signaled, but object will interpret this as
		 *  also as releasing the resource
		 */
		bool remove(Slot &slot);


		///Notifies one slot
		/**
		 * @param fn Function is called during notification - note that
		 * 	function is called while internal lock is held allowing it to
		 * 	manipulate with slot atomically. Function receives R/W pointer to
		 * 	the slot as argument
		 * @retval true notified
		 * @retval false empty queue
		 *
		 * @note  Note that notifications
		 *  and cancellation are mutual exclusive operations and may block each other. Do not
		 *  call remove() in notification event (causing deadlock)
		 *
		 */
		template<typename Fn>
		bool notifyOne(Fn fn);

		///Notifies all slots
		/**
		 * @param fn Function is called during notification - note that
		 * 	function is called while internal lock is held allowing it to
		 * 	manipulate with slot atomically.Function receives R/W pointer to
		 * 	the slot as argument
		 * @retval true notified
		 * @retval false empty queue
		 *
		 * @note  Note that notifications
		 *  and cancellation are mutual exclusive operations and may block each other. Do not
		 *  call remove() in notification event (causing deadlock)
		 *
		 */
		template<typename Fn>
		bool notifyAll(Fn fn);


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


	template<class Fn>
	bool SyncPt::notifyOne(Fn fn) {
		Synchronized<FastLock> _(queueLock);
		Slot *p = popSlot_lk();
		if (p == 0) return false;
		fn(p);
		p->notify(0);
		return true;
	}

	template<class Fn>
	bool SyncPt::notifyAll(Fn fn) {
		Synchronized<FastLock> _(queueLock);
		bool suc = false;
		Slot *p = curQueue;
		curQueue = 0;
		while (p) {
			fn(p);
			Slot *q = p;
			p = p->next;
			q->notify();
			suc = true;
		}
		return suc;
	}



}


#if 0

#include "../base/types.h"
#include "sleepingobject.h"
#include "atomic_type.h"
#include "timeout.h"
#include "criticalSection.h"

namespace LightSpeed {

	class SyncPt;

	class SyncSlot;
	///base class of SyncSlot. Allows to register thread on SyncPt
	class SyncSlotBase {
	public:

		///State of slot
		enum SlotState {
			///State is clean - it can be registered to the SyncPt
			clean = 0,
			///State is waiting - slot is registered and waiting for notification
			waiting = 1,
			///Signaled state - slot has been signaled and removed from SyncPt
			signaled = 2
		};

		///Creates raw sync.slot
		/**
		 * @param obj object, which will be notified through this slot
		 * @param reason reason used with notification
		 */
		SyncSlotBase(ISleepingObject &obj, natural reason = 0)
			:waitObj(obj),reason(reason), state(clean),next(0) {}

		///Retrieves state of slot
		/**
		 * @return state of slot: SlotState
		 */
		SlotState getState() const {return state;}

		///Returns true, if slot is signaled
		/** allows to use slot as boolean variable.
		 */
		operator bool() const {return state == signaled;}

		bool operator !() const {return state != signaled;}

		///Sets slot signaled manually
		/**
		 * @note function doesn't send notification.Only changes state.
		 * Implementation of SyncPt may set this state during registering
		 * the slot when it finds, that there is no reason to wait. You
		 * have to check state of slot before you start waiting
		 */
		void setSignaled() {
			if (state != waiting) state = signaled;
		}
		void reset() {
			if (state != waiting) state = clean;
		}


		///Retrieves id of thread associated with this object
		/**
		 * @return associated thread.
		 *
		 * @note function can return valid result only if previous setThreadId has
		 * been called. Otherwise, result is undefined. Function is used
		 * by several locks where ownership is bound to the thread.
		 */
		atomic getThreadId() const {return threadId;}
		///Associates this object with the thread using atomic thread identifer
		/**
		 * @param thrId id of thread associated with this object
		 */
		void setThreadId(atomic thrId) {threadId = thrId;}


	protected:

		ISleepingObject &waitObj;
		natural reason;
		SlotState state;
		SyncSlotBase *volatile next;
		atomic threadId; ///<reserved for threadId - not set by constructor

		friend class SyncPt;
	};


	///Basic synchronization between threads
	/** This object is main part of all high-level synchronization objects.
	 * It contains the container of threads, which waiting for notification
	 * from another thread. When other thread wants to notify waiting threads,
	 * it can notify first or all objects in the container.
	 *
	 * Thread that wish to wait creates SyncSlotBase object which is registered
	 * on the SyncPt and then it can be signaled by some other thread. The thread can
	 * assign itself to the SyncSlotBase as ISleepingObject and then it is able
	 * to sleep until notification is caught causing thread wakening up.
	 *
	 * This allows to the thread to wait on multiple sync.points while first
	 * notification wakes the waiting thread and it is able to find out, which
	 * notification was it
	 *
	 * If thread wish to wait only one sync.point at time, it can use
	 * method wait() without need to creating SyncSlot.
	 */
	class SyncPt {
	public:


		SyncPt();

		///Registers slot to the sync.point
		/**
		 * @param slot slot to register
		 * @exception InvalitParamException thrown, is slot is already waiting
		 */
		virtual void add(SyncSlotBase &slot);

		///Removes slot from the sync.point
		/**
		 * @param slot slot to remove. If slot has not been registered, function
		 * 	does nothing. If slot is waiting, but not at this sync point,
		 *  function doesn't change state of slot from waiting to clean.
		 *
		 *  You can use SyncSlotBase::getState() to test, whether slot has been
		 *  removed.
		 *
		 *  @note in connection with object based on SyncPt, removing slot
		 *  from the SyncPt object is interpreted as give up of waiting. During
		 *  removin, slot may be signaled, but object will interpret this as
		 *  also as releasing the resource
		 */
		virtual bool remove(SyncSlotBase &slot);



		enum WaitInterruptMode {

			///uninterruptable
			/** waiting cannot be interrupted before event on timeout reached */
			uninterruptable,
			///interrupt waiting on set exit flag
			/** waiting will be interrupted, when Thread::setExitFlag is called,
			 *
			 * @note you cannot continue wait, because exit flag remains active
			 *  */
			interruptOnExit,
			///interrupt wait on another event
			/**
			 * Interrups waiting on any event caught by wakeUp() method.
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
		 * @param tm specifies timeout
		 * @param mode specifies, how waiting can be interrupted
		 * @retval true wait success
		 * @retval false timeout
		 */
		bool wait(const Timeout &tm, WaitInterruptMode mode = uninterruptable);

		virtual ~SyncPt() {}
	protected:
		///Notifies one waiting slot
		/** It takes first added and sends notification to it. Then
		 * slot is removed from the container
		 *
		 * @retval true notified
		 * @retval false empty contianer
		 * */
		virtual SyncSlotBase *notifyOne();
		///Notifies all waiting slots
		/**
		 * Notifies all, and removes them
		 *
		 * @retval true notified
		 * @retval false empty contianer
		 *
		 */
		virtual bool notifyAll();

		SyncSlotBase *volatile first;
		SyncSlotBase *volatile added;
		typedef CriticalSection LockImpl;
		mutable LockImpl inrlk;
		typedef Synchronized<LockImpl> Lock;


		void reorder();
		SyncSlotBase *getFirst();
		SyncSlotBase *getNew();
		void append(SyncSlotBase *p);
		void signalSlot(SyncSlotBase *p);


	};


	///Implements waitaible slot which can be used with SyncPt
	/**
	 * Using this slot, you can wait or receive notifications from SyncPt
	 *
	 * Note that SyncSlot doesn't inherit SyncSlotBase. This is by design decision.
	 * SyncSlot is always connected with one instance SyncPt and cannot be used with other
	 * instance. So class doesn't inherit SyncSlotBase to prevent manual registration
	 * using method SyncPt::add() and SyncPt::remove()
	 */
	class SyncSlot {
	public:

		///Creates sync.slot forwarding notification to the specified ISleepingObject
		/**
		 *
		 * @param wobj which object will be notified
		 * @param syncPt reference to instance of syncPt
		 *
		 * SyncSlot instance is automatically removed on destructor regardless
		 * on its state.
		 */
		SyncSlot(ISleepingObject &wobj, SyncPt &syncPt);
		///Creates sync.slot which will wake up current thread
		/**
		 * @param syncPt reference to instance of syncPt. When object is created,
		 * you can call Thread::sleep() to wait for the event. But don't forget
		 * to check whether slot is signaled
		 *
		 * @code
		 * SyncSlot s(mysyncpt);
		 * while (!s) Thread::sleep(naturalNull);
		 * @endcode
		 *
		 * SyncSlot instance is automatically removed on destructor regardless
		 * on its state.
		 */
		SyncSlot(SyncPt &syncPt);
		~SyncSlot() {
			syncPt.remove(ssb);
		}

		///Retrieves state of slot
		/**
		 * @return state of slot: SyncSlotBase::SlotState
		 */
		SyncSlotBase::SlotState getState() const {return ssb.getState();}

		///Returns true, if slot is signaled
		/** allows to use slot as boolean variable.
		 */
		operator bool() const {return ssb;}
		///Returns true, if slot is signaled
		/** allows to use slot as boolean variable.
		 */
		bool operator!() const {return !(bool)ssb;}


		bool wait(const Timeout &tm, SyncPt::WaitInterruptMode mode);


	protected:
		SyncSlotBase ssb;
		SyncPt &syncPt;

		friend class SyncPt;
	};

}

#endif
#endif /* LIGHTSPEED_MT_SYNCPT_H_ */
