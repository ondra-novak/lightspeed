/*
 * netStreamSource.h
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_WINDOWS_NETSTREAMSOURCE_H_
#define LIGHTSPEED_BASE_WINDOWS_NETSTREAMSOURCE_H_
#include "../streams/netio_ifc.h"
#include "netStream.h"
#include "../containers/optional.h"
#include "../memory/staticAlloc.h"


namespace LightSpeed {



class WindowsNetAccept: public NetworkResourceCommon<INetworkStreamSource>,
						public INetworkSocket {
public:
	WindowsNetAccept(PNetworkAddress addr,
			natural count ,natural timeout,
			natural streamDefTimeout,
			INetworkServices *svc);
	~WindowsNetAccept();

	virtual bool hasItems() const ;
	virtual PNetworkStream getNext() ;
	virtual PNetworkAddress getPeerAddr() const ;
	virtual PNetworkAddress getLocalAddr() const ;

	virtual integer getSocket(int idx) const {return idx?-1:acceptSock;}
	virtual natural getDefaultWait() const {return waitForInput;}

protected:

	UINT_PTR acceptSock;
	mutable PNetworkAddress localAddr;
	PNetworkAddress remoteAddr;
	natural count;
	natural streamDefTimeout;
	Pointer<WaitHandler> handler;
	INetworkServices *svc;
private:
	WindowsNetAccept(const WindowsNetAccept &);
	WindowsNetAccept &operator=(const WindowsNetAccept &);
	natural doWait(natural waitFor, natural timeout) const;
};

class WindowsNetConnect: public NetworkResourceCommon<INetworkStreamSource>,
					   public INetworkSocket {
public:

	WindowsNetConnect(PNetworkAddress addr,
			natural count ,natural timeout,
			natural streamDefTimeout,
			INetworkServices *svc);
	virtual ~WindowsNetConnect();

	virtual bool hasItems() const ;
	virtual PNetworkStream getNext() ;
	virtual PNetworkAddress getPeerAddr() const ;
	virtual PNetworkAddress getLocalAddr() const ;

	virtual integer getSocket(int idx) const;
	virtual natural getDefaultWait() const {return waitForOutput;}

protected:
	PNetworkAddress localAddr;
	PNetworkAddress remoteAddr;
	natural count;
	natural streamDefTimeout;
	mutable natural readySocket;

	static const natural maxWatchSockets = 4;
	mutable AutoArray<UINT_PTR, StaticAlloc<maxWatchSockets> > asyncSocket;

	void setupSocket() const;

	PNetworkStream makeConnect();
	INetworkServices *svc;
	virtual natural doWait(natural waitFor, natural timeout) const;

};

}

#endif /* LIGHTSPEED_BASE_WINDOWS_NETSTREAMSOURCE_H_ */
