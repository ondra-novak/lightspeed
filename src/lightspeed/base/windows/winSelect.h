/*
 * eWinSelect.h
 *
 *  Created on: 14. 9. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_LINUX_EWinSelect_H_
#define LIGHTSPEED_BASE_LINUX_EWinSelect_H_

#include <WinSock2.h>
#include "../../mt/sleepingobject.h"
#include "../../mt/timeout.h"
#include "../containers/autoArray.h"
#include "../containers/sort.h"
#include "../containers/map.h"

namespace LightSpeed {

///WinSelect implements one-shot select operation that uses poll internally
/** Because LightSpeed uses one-shot monitoring for all its streams, object also performs
 * one-shot monitoring.
 *
 * You can charge monitoring on specified socket (fd) and once the event is observed, the
 * function wait will return it and it also disables future monitoring, until the descriptor
 * is charged again
 *
 * Function is not MT safe (only exception is function wakeUp)
 */
class WinSelect : public ISleepingObject{
public:
	WinSelect();
	~WinSelect();


	///Charges monitoring on the descriptor
	/**
	 * @param fd number of descriptor
	 * @param waitFor wait flags - they are defined in INetworkResource
	 * @param tm timeout
	 * @param userData user specified data
	 */
	void set(int fd, natural waitFor, Timeout tm, void *userData);

	///Removes monitoring on the descriptor
	/**
	 * @param fd number of descriptor
	 *
	 * @note To achieve best performance, descriptor is not removed from the epoll service. Function
	 * just enables ignoring all future events
	 */
	void unset(int fd);

	///Result after waiting
	struct Result {
		///id of descriptor
		SOCKET fd;
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

		waitNone
	};

	///Waits for event, wakeup or timeout
	/**
	 * @param tm timeout, can be set to null to achieve infinite waiting
	 * @param result once function exits, there is stored result of waiting
	 * @return reason why function exits, see WaitStatus
	 */
	WaitStatus wait(const Timeout &tm, Result &result);

	WaitStatus walkResult(Result &result);

	///Interrupts waiting
	/** Function can be called from another thread. Function interrupts any current or future waiting.
	 *
	 * If there is no active wait, function schedules interruption to future wait
	 *
	 * @param reson specifies reason. Value is unrealible when wakeUp is called multiple times between each wait
	 */
	void wakeUp(natural reason) throw();


	///Returns user data associated with the descriptor
	void *getUserData(int fd) const;

	///Cancels monitoring for all descriptors and allows to caller perform some cleanup
	/**
	 * @param cleanUp called for each active descriptor. Function has two parameters. First parameter
	 * is an id of descriptor, second parameter is pointer to user data as it was passed to function set()
	 *
	 * Function is useful to perform cleanup operation
	 */
	template<typename Fn>
	void cancelAll(Fn cleanUp);

protected:

	typedef struct pollfd POLLFD;

	SOCKET wakefd;
	natural reason;
	SOCKADDR_IN wakeaddr;


	struct FdInfo {
		SOCKET socket;
		Timeout timeout;
		natural waitFor;
		void *userData;
		natural activeIndex;


		FdInfo() :socket(0), waitFor(0), userData(0), activeIndex(naturalNull) {}
	};



	typedef Map<SOCKET, FdInfo> SocketMap;
	typedef AutoArray<FdInfo *>ActiveSockets;

	SocketMap socketMap;
	ActiveSockets activeSockets;


	class FdSetEx {
	public:

		FdSetEx();
		~FdSetEx();
		FdSetEx(const FdSetEx &other);
		FdSetEx &operator=(const FdSetEx &other);

		operator fd_set *() { return fdset; }
		operator const fd_set *() const { return fdset; }
		fd_set *operator ->() { return fdset; }
		const fd_set *operator ->() const { return fdset; }

		void reserve(natural count);
		void reset();
		void add(SOCKET s);
		natural getCount() const { return fdset ? fdset->fd_count : 0; }
		natural getAllocated() const { return allocated; }
	protected:
		natural allocated;
		fd_set *fdset;
	};

	FdSetEx readfds, writefds, exceptfds;

	natural rdpos;
	natural wrpos;
	natural epos;

private:
	int getFd(const FdInfo* finfo);
	WaitStatus procesResult(const FdSetEx &readfds, natural rdpos, natural type, Result &result);
};

} /* namespace LightSpeed */

template<typename Fn>
inline void LightSpeed::WinSelect::cancelAll(Fn cleanUp) {
	for (natural i = 0; i < socketMap.length(); i++) {
		if (socketMap[i].waitFor != 0) {
			cleanUp((int)i, socketMap[i].userData);
			socketMap(i).waitFor = 0;
		}
	}
}

#endif /* LIGHTSPEED_BASE_LINUX_EWinSelect_H_ */
