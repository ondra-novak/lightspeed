/*
 * gate.h
 *
 *  Created on: 21.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_GATE_H_
#define LIGHTSPEED_MT_GATE_H_

#include "syncPt.h"

namespace LightSpeed {

	///Simple gate
	/** Gate is synchronization object. It is used to synchronize two or more threads.
	 *
	 *  Gate can be in two states. "opened" and "closed". In "opened" state, any thread can go through
	 *  without interruption. In state "closed" all threads must stop on gate and are blocked, until gate
	 *  is opened. Opening gate causes, that all blocked threads are released.
	 *
	 *  To go through gate, use method wait(). To control gate, use methods open() and close()
	 *
	 */
	class Gate {
	public:

		typedef SyncPt::Slot Slot;

		enum State {
			///gate is opened, threads will not be blocked
			stateOpen,
			///gate is closed, all threads will be blocked
			stateClose
		};

		///Initializes gate and sets initial state
		/**
		 * @param state initial state of gate
		 */
		Gate(State state = stateClose):state(state) {}

		///Opens gate, releases any waiting thread
		void open();

		///Closes gate
		void close();

		///Asynchronous wait
		/**
		 * To asynchronous wait, thread must initialize SyncPt's slot and register this slot to
		 * asynchronous wait. Once gate is opened, slot notifies thread and becomes signaled
		 * @param slot
		 */
		void waitAsync(Slot &slot);


		///Cancels asychronous wait
		/**
		 * @param slot slot to cancel
		 * @retval true canceled
		 * @retval false failed, probably not necesery
		 */
		bool cancelWaitAsync(Slot &slot);

		bool isOpened() const {return state == stateOpen;}

		bool wait(const Timeout& tm, SyncPt::WaitInterruptMode mode);
		bool wait(const Timeout& tm);


	protected:
		friend class SyncPt::Slot;
		
		void cancelSlot(Slot &sl) {cancelWaitAsync(sl);}

		atomic state;
		SyncPt sp;
	private:
		Gate(const Gate &other);

	};

	inline void Gate::open()
	{
		writeRelease(&state,stateOpen);
		sp.notifyAll();
	}



	inline void Gate::close()
	{
		writeRelease(&state,stateClose);
	}




	inline void Gate::waitAsync(Slot& slot) {
		if (readAcquire(&state) == stateOpen)
			slot.notify();
		else {
			sp.add(slot);
			if (readAcquire(&state) == stateOpen) {
				sp.remove(slot);
				slot.notify();
			}
		}
	}

	inline bool Gate::cancelWaitAsync(Slot& slot) {
		return sp.remove(slot);
	}

	inline bool Gate::wait(const Timeout& tm, SyncPt::WaitInterruptMode mode) {
		Slot s;
		waitAsync(s);
		return sp.wait(s,tm,mode) || !sp.remove(s);

	}

	inline bool Gate::wait(const Timeout& tm) {
		Slot s;
		waitAsync(s);
		return sp.wait(s,tm) || !sp.remove(s);

	}
}


#endif /* LIGHTSPEED_MT_GATE_H_ */
