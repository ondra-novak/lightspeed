#pragma once

#ifndef _LIGHTSPEED_BASE_STREAM_NETIO_POLL
#define _LIGHTSPEED_BASE_STREAM_NETIO_POLL
#include "../../mt/sleepingobject.h"

 namespace LightSpeed {
class Timeout;

	///Interface to various implementation of Poll/Select for the sockets
	/** In most of cases, you will not need this interface, because monitoring of the network streams
	 * is handled by INetworkEventListener. This interface is used internally where underlying sockets
	 * take in place
	 *
	 * @tparam Socket type of socket
	 *
	 * @note objects inplementing this interface are not requered to be MT safe. Just only function
	 * wakeUp must allow MT Safety, so other threads can call it without harming the internals
	 */
	template<typename Socket>
	class INetworkSocketPoll: public ISleepingObject {
	public:

		///Result after waiting
		struct Result {
			///id of descriptor
			Socket fd;
			union {
				///which events happened on the descriptor
				natural flags;
				///for result waitWakeUp, there is stored reason
				natural reason;
			};
			///user data associated with the descriptor
			void *userData;
		};

		enum WaitStatus {
			///wait timeouted, result structure was not changed
			waitTimeout,
			///event detected, result structure contains details
			waitEvent,
			///wakeUp called, result structure contains reason.
			waitWakeUp,
			///internal wait state, should not appear outside of the object
			waitInternal1,
			///internal wait state, should not appear outside of the object
			waitInternal2,
			///internal wait state, should not appear outside of the object
			waitInternal3



		};

		///Callback used in cancelAllVt.
		/** It is better to use cancelAll() */
		class ICancelAllCb {
		public:
			virtual void operator()(Socket fd, void *userData) const = 0;
		};

		virtual ~INetworkSocketPoll() {}

		///Enables monitoring on the socket.
		/**
		 * @param fd socket handle
		 * @param waitFor wait flags (as defined in INetworkResource)
		 * @param tm timeout how long socket will be monitored
		 * @param userData user data associated with the request
		 */
		virtual void set(Socket fd, natural waitFor, const Timeout &tm, void *userData) = 0;
		///Disables monitoring on the socket
		/**
		 * @param fd socket handle
		 */
		virtual void unset(Socket fd) = 0;
		///Starts monitoring
		/** Function wait for any event that may happen on any registered socket. If event detected,
		 * informations including the socket handle is returned in "result" structure. Type of
		 * event is returned
		 * @param tm timeout how long wait on event
		 * @param result structure filled with details about the event
		 * @return reason why monitoring was interrupted
		 */
		virtual WaitStatus wait(const Timeout &tm, Result &result) = 0;
		///Cancel the monitoring. Function wait() returns waitWakeUp.
		/**
		 * @param reason user defined value as reason. If there are multiple threads with ability
		 * to call this function, reason can be unrealibite, because it is not queued, each call
		 * overwrites previous value.
		 */
		virtual void wakeUp(natural reason) throw() = 0;
		///Retrieves user data associated with the socket
		/**
		 * @param fd socket
		 * @return function return NULL for non-registered socket, or if the pointer was not set. Otherwise
		 * function returns pointer associated with the socket
		 */
		virtual void *getUserData(int fd) const = 0;

		///Cancels all monitoring, giving the caller a chance to perform cleanup
		/**
		 * Wallks all registered sockets and calls callback. Function expects, that callback whill
		 * perform cleanup. After cleanup, all socket are unset
		 * @param cb callback as interface
		 */
		virtual void cancelAllVt(const ICancelAllCb &cb) = 0;


		///Cancels all monitoring, giving the caller a chance to perform cleanup
		/**
		 * Wallks all registered sockets and calls callback. Function expects, that callback whill
		 * perform cleanup. After cleanup, all socket are unset
		 * @param cleanup function called every socket.
		 */
		template<typename Fn>
		void cancelAll(Fn cleanUp) {
			class A: public ICancelAllCb {
			public:
				virtual void operator()(Socket fd, void *userData) const {
					fn(fd,userData);
				}
				A(Fn fn):fn(fn) {}
				Fn fn;
			};
			cancelAllVt(A(cleanUp));
		}
	};
}

#endif
