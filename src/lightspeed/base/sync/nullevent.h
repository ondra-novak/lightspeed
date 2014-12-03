/*
 * nullevent.h
 *
 *  Created on: 26.9.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_SYNC_NULLEVENT_H_
#define LIGHTSPEED_SYNC_NULLEVENT_H_
#pragma once

namespace LightSpeed {

	class Timeout;


	///Implements null-version of event object
	/** null-version means, that object is only interface prototype with
	 * no function. It simply do nothing, but defines basic interface
	 * for all objects of the same type.
	 *
	 * Event is synchronization object, which can block one thread
	 * until it is notified by another thread or event.
	 *
	 * Another name for this object is  "waitable object" One resource
	 * is blocked (and must wait) until another resource notifies
	 * object. If there is not blocked resource, and notify arrives,
	 * event remains opened and will not block next wait() request.
	 * If there are more wait requests, Event should reguster them in
	 * the queue.
	 *
	 *
	 */
	class NullEvent {
	public:

		///Waits on event
		/** Implementation is empty, but object implementing this interface
		 * should block the caller, until event is notified
		 * @retval true success (not case of this implementation)
		 * @retval false failure - or not implemented
		 *
		 */
		bool wait()  {throwUnsupportedFeature(THISLOCATION,this,"wait()");throw;}

		///Waits on event specifying the timeout
		/**
		 * @param tm timeout specification
		 * @retval true success
		 * @retval false failure, timed out, or not supported
		 */
		bool wait(const Timeout &tm)  {throwUnsupportedFeature(THISLOCATION,this,"wait()");throw;}

		///Notifies even object
		void notify() {}


	};

}


#endif /* NULLEVENT_H_ */
