/*
 * NetworkEventListener.h
 *
 *  Created on: 13.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_LINUX_NETWORKEVENTLISTENER_H_
#define LIGHTSPEED_LINUX_NETWORKEVENTLISTENER_H_

#include "winSelect.h"
#include "../actions/msgQueue.h"
#include "../containers/linkedList.h"
#include "../memory/clusterAllocFactory.h"
#include "../../mt/thread.h"
#include "../memory/poolalloc.h"
#include "../streams/netio_ifc.h"



namespace LightSpeed {
class WinNetworkEventListener: public INetworkEventListener, protected MsgQueue {
public:
	WinNetworkEventListener();
	virtual ~WinNetworkEventListener();

	virtual void set(const Request &request);


protected:

	virtual void notify() ;

	class RequestMsg: public MsgQueue::AbstractMsg, public DynObject {
	public:
		RequestMsg(WinNetworkEventListener &owner, const Request &r):owner(owner),r(r) {}
	protected:
		WinNetworkEventListener &owner;
		virtual void run() throw();
		virtual void done() throw();
		Request r;
	};

	struct FdListener {
		ISleepingObject *notify;
		Timeout waitTimeout;
		natural waitMask;

		FdListener(const Request &r)
			:notify(r.notify),waitTimeout(r.timeout_ms),waitMask(r.waitFor) {}
	};

	typedef AutoArray<FdListener, SmallAlloc<4> > ListenerMap;

	class FdData: public DynObject {
	public:
		ListenerMap listeners;

	};

	class CleanUpProc;




	Thread worker;

	WinSelect fdSelect;
	NodeAlloc factory;
	PoolAlloc reqAlloc;
	FastLock lock;


	void doRequest(const Request &r);
	void doRequestWithSocket(const Request &r, SOCKET sck);
	void updateFdData(FdData *dta,int sck);

	void workerProc();
	void cleanFd(int fd, void *data);




};

}
#endif /* LIGHTSPEED_LINUX_NETWORKEVENTLISTENER_H_ */
