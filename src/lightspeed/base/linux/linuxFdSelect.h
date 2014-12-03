/*
 * linuxFdSelect.h
 *
 *  Created on: 13.6.2012
 *      Author: ondra
 */

#ifndef LIGHTSPEED_LINUX_LINUXFDSELECT_H_
#define LIGHTSPEED_LINUX_LINUXFDSELECT_H_

#include "../containers/autoArray.h"
#include "../../mt/timeout.h"
#include "../containers/map.h"
#include "../streams/netio_ifc.h"


using LightSpeed::natural;

typedef struct pollfd POLLFD;


namespace LightSpeed {

///Implements waiting for set of descriptors.
/** Depend on platform, implementation can be differ. Basic implementation uses poll() to wait for descriptors. Linux implementation
 * can use epoll() to implement this.
 *
 */
class LinuxFdSelect: public ISleepingObject {
public:

	///waiting timeouted
	static const natural waitTimeout = INetworkResource::waitTimeout;
	///waiting for input (reading)
	static const natural waitForInput = INetworkResource::waitForInput;
	///waiting for output (writing)
	static const natural waitForOutput = INetworkResource::waitForOutput;
	///waiting for network exception
	static const natural waitForException = INetworkResource::waitForException;

	///Information about descriptor inside of the object
	struct FdInfo {
		///file descriptor
		int fd;
		///mask specifies which events will be watched. Events are defined int this class
		/** When structure is read from object, field contains mask of recorded events		 */
		natural waitMask;
		///pointer to user data
		void *data;
		///timeout definition for this descriptor
		Timeout tmout;

		///ctor
		FdInfo() {}
		///ctor
		FdInfo(int fd, natural waitMask,void *data,const Timeout &tmout):fd(fd),waitMask(waitMask),data(data),tmout(tmout) {}
	};

	///ctor
	LinuxFdSelect();
	~LinuxFdSelect();

	///Sets new descriptor for watching events
	/**
	 * @param fd file descriptor id
	 * @param waitMask which events will be watched
	 * @param data user defined pointer
	 * @param tmout timeout
	 *
	 * @note if descriptor is already registered, function overwrites settings. Setting waitMask to zero causes, that
	 *  descriptor will be treat as timeouted and returned by getNext() and peek() functions
	 */
	void set(int fd, natural waitMask,void *data, Timeout tmout);
	///Removes descriptor from the object
	/**
	 * @param fd descriptor toremove
	 * @return user data stored with descriptor. This allows to easy locate and cleanup user structures
	 */
	void *unset(int fd);
	///Sets descriptor indirectly using FdInfo structure
	/**
	 * @param fdinfo contains everything need to start monitoring the descriptor
	 */
	void set(const FdInfo& fdinfo);

	///test, whether descriptor's monitoring is set
	/**
	 * @param fd descriptor identifier
	 * @retval true descriptor is monitored
	 * @return fasle descriptor is not monitored
	 */
	bool isSet(int fd);

	///retrieves user data associated with the descriptor
	/**
	 * @param fd descriptor identifier
	 * @return pointer stored with the descriptor. If descriptor is not being watched, returns NULL
	 */
	void *getData(int fd);

	///retrieves user data associated with the descriptor
	/**
	 * @param fd descriptor identifier
	 * @param exists reference to variable that receives true, if descriptor exists in object, and false otherwise
	 * @return pointer stored with the descriptor. If descriptor is not being watched, returns NULL
	 */
	void *getData(int fd, bool &exists);

	///determines, whether there are descriptor to return by getNext() or peek()
	/**
	 * @retval true there are descriptors to return. Note that function return true if there are at least one
	 * descriptor in any state. To remove descriptors, you need to call unset(). Function will never return false
	 * if descriptors are not unset during iterating.
	 *
	 * @see getNext, peek
	 */
	bool hasItems() const;
	///retrieves next signaled descriptor
	/**
	 * Function blocks, if there is no signaled descriptors until some signaled appear. Note that timeout on particular
	 * descriptor is also signal.
	 *
	 * @return reference to signaled descriptor
	 */
	const FdInfo &getNext();
	///retrieves next signaled descriptor without advancing iterator
	/**
	 * Function retrieves next signaled descriptor without advancing iterator. Next call of peek() or getNext() will return
	 * the same descriptor
	 * @return reference to signaled descriptor
	 */
	const FdInfo &peek() const;

	///drops all waiting descriptors
	/** All descriptors are market dropped and function getNext() can be used to remove them without waiting
	 *  Note, function is not MT safe, don't use it to stop waiting. Will not work. You can drop only between waiting
	 */
	void dropAll();

	///Enables or disables function wakeUp
	/** You have to call this function before wakeUp() can be called. Function prepares
	 *  internal structures to receive wakeUp request. Without this preparation, wakeUp() will unable
	 *  to perform operation
	 *  @param enable true to enable, false to disable. Duplicated request are ignored. Default is disabled
	 */
	void enableWakeUp(bool enable);
	///Interrupts waiting allowing to force waiting thread do something extra.
	/** Function can be called from different thread
	 *
	 * @param userData data sent with the request. Data must be read by function isWakeUpRequest().
	 *
	 * @note before using this function, it must be enabled. See enableWakeup()
	 *
	 */
	void wakeUp(natural reason) throw ();

	///Determines whether descriptor of event is wakeUp request
	/**
	 * Function must be called immediatelly after getNext() to filter out wakeUp requests. This is
	 * need only when wakeUp is enabled (see enableWakeUp() function).
	 *
	 * @param fdInfo descriptor returned by getNext()
	 * @param reason variable, that receives value "reason" used while calling wakeUp(). Note that in compare
	 *  to wakeUp on threads, in this case, all events are captured and delivered, not lastone only.
	 * @retval true wakeUp request read and extracted
	 * @retval false this is not wakeUp request.
	 */
	bool isWakeUpRequest(const FdInfo &fdInfo, natural &reason) const;


	bool waitForEvent(const Timeout &timeout) const;
protected:

	struct FdExtra {
		void *data;
		Timeout tmout;
	};

	mutable AutoArray<POLLFD> fdList;
	AutoArray<FdExtra> fdExtra;

	Map<int, natural> fdIndex;
	mutable natural curPos;

	mutable FdInfo retVal;

	int cancelIn;
	int cancelOut;
	atomic pendingWakeup;
	natural reason;

	void removeFd(natural pos);
	natural findSignaled(bool noBlock = false) const;
	bool doWaitPoll(const Timeout &tm) const;

	LinuxFdSelect(const LinuxFdSelect &);
	LinuxFdSelect &operator=(const LinuxFdSelect &);

};
}



#endif /* LIGHTSPEED_LINUX_LINUXFDSELECT_H_ */
