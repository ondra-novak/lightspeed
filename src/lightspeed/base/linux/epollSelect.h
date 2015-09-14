/*
 * epollSelect.h
 *
 *  Created on: 14. 9. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_LINUX_EPOLLSELECT_H_
#define LIGHTSPEED_BASE_LINUX_EPOLLSELECT_H_
#include "../../mt/sleepingobject.h"
#include "../../mt/timeout.h"
#include "../containers/autoArray.h"
#include "../containers/sort.h"

namespace LightSpeed {

///EpollSelect implements one-shot select operation that uses epoll internally
/** Because LightSpeed uses one-shot monitoring for all its streams, object also performs
 * one-shot monitoring.
 *
 * You can charge monitoring on specified socket (fd) and once the event is observed, the
 * function wait will return it and it also disables future monitoring, until the descriptor
 * is charged again
 *
 * Function is not MT safe (only exception is function wakeUp)
 */
class EPollSelect : public ISleepingObject{
public:
	EPollSelect();
	~EPollSelect();


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
		int fd;
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
		waitWakeUp
	};

	///Waits for event, wakeup or timeout
	/**
	 * @param tm timeout, can be set to null to achieve infinite waiting
	 * @param result once function exits, there is stored result of waiting
	 * @return reason why function exits, see WaitStatus
	 */
	WaitStatus wait(const Timeout &tm, Result &result);

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

	int epollfd;
	int wakeIn;
	int wakeOut;
	natural reason;

	struct TmInfo;

	struct FdInfo {
		Timeout timeout;
		natural waitFor;
		void *userData;
		TmInfo *tmRef;


		FdInfo():waitFor(0),userData(0),tmRef(0) {}
	};


	struct TmInfo {
		FdInfo *owner;

		TmInfo(FdInfo *owner);
		TmInfo(const TmInfo &origin);
		TmInfo &operator=(const TmInfo &origin);

	};

	struct TimeoutCmp {
		bool operator()(const TmInfo &a, const TmInfo &b) const;
	};

	typedef AutoArray<FdInfo> SocketMap;
	typedef AutoArray<TmInfo> TimeoutMap;
	typedef HeapSort<TimeoutMap, TimeoutCmp> TimeoutHeap;

	SocketMap socketMap;
	TimeoutMap timeoutMap;
	TimeoutHeap timeoutHeap;


private:
	int getFd(const FdInfo* finfo);
	void addNewTm(FdInfo *owner);
	void removeOldTm(FdInfo *owner);
	void clearHeap();
};

} /* namespace LightSpeed */

template<typename Fn>
inline void LightSpeed::EPollSelect::cancelAll(Fn cleanUp) {
	for (natural i = 0; i < socketMap.length(); i++) {
		if (socketMap[i].waitFor != 0) {
			cleanUp(i, socketMap[i].userData);
			socketMap(i).waitFor = 0;
			socketMap(i).tmRef = 0;
		}
	}
	clearHeap();
}

#endif /* LIGHTSPEED_BASE_LINUX_EPOLLSELECT_H_ */
