/*
 * ePollListener.h
 *
 *  Created on: 9. 9. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_LINUX_EPOLLLISTENER_H_
#define LIGHTSPEED_BASE_LINUX_EPOLLLISTENER_H_
#include <algorithm>
#include <utility>

#include "../../mt/sleepingobject.h"
#include "../../mt/thread.h"
#include "../streams/netio_ifc.h"
#include "../actions/msgQueue.h"
#include "../containers/map.h"
#include "../containers/sort.h"
#include "../memory/poolalloc.h"
#include "../timestamp.h"



namespace LightSpeed {

class EPollListener_SwapHelper;

class EPollListener: public INetworkEventListener, protected MsgQueue  {
public:
	EPollListener();
	~EPollListener();

	virtual void set(const Request &request);

protected:

	virtual void notify() ;


	friend class EPollListener_SwapHelper;

private:

	Thread worker;


	void workerProc();

	void onRequest(const Request &request);

	int wakePipeFds[2];

	FastLock lock;


	class RefCntPtrFdDesc;

	class FdDesc: public DynObject, public RefCntObj {
	public:
		const Timeout timeout;
		ISleepingObject * const  ntf;
		const RefCntPtr<INetworkResource> rsrc;

		FdDesc(RefCntPtr<INetworkResource> rsrc, const Timeout &timeout, ISleepingObject *wk);

		///position in socketTimeouts
		/** Position is updated everytime the record is moved due ordering. It allows
		 * to find record faster. Pointer is never referenced, it is used to calculate
		 * index. If index is out of range, full search is performed
		 */
		RefCntPtrFdDesc *hintPos;
	};

	class RefCntPtrFdDesc: public RefCntPtr<FdDesc> {
	public:
		RefCntPtrFdDesc(const RefCntPtr<FdDesc> &other):RefCntPtr<FdDesc>(other) {other->hintPos = this;}
		RefCntPtrFdDesc(const RefCntPtrFdDesc &other):RefCntPtr<FdDesc>(other) {other->hintPos = this;}
		RefCntPtrFdDesc &operator=(const RefCntPtrFdDesc &other) {
			RefCntPtr<FdDesc>::operator=(other);
			other->hintPos = this;
			return *this;
		}
	};

	struct TimeoutOrder {
		bool operator()(const RefCntPtr<FdDesc> &a, const RefCntPtr<FdDesc> &b) const;
	};

	typedef Map<int, RefCntPtr<FdDesc> > SocketMap;
	typedef AutoArray<RefCntPtrFdDesc > SocketHeapCont;
	typedef HeapSort<SocketHeapCont, TimeoutOrder> SocketHeap;
	SocketMap socketMap;
	SocketHeapCont socketHeapCont;
	SocketHeap socketTimeouts;


	PoolAlloc alloc;
	int epoll_fd;

	static void regFd(int epoll, natural waitFor, int fd, FdDesc *desc);
	static void unregFd(int epoll, int fd);
	void eraseReg(int fd);
	void addReg(int fd, RefCntPtr<FdDesc> fdesc, natural waitFor);
	void eraseReg(const RefCntPtr<FdDesc>& fdesc);
	void eraseTimeout(const RefCntPtr<FdDesc>& fdesc);
};

} /* namespace LightSpeed */



#endif /* LIGHTSPEED_BASE_LINUX_EPOLLLISTENER_H_ */
