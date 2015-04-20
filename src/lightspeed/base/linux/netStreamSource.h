/*
 * netStreamSource.h
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_LINUX_NETSTREAMSOURCE_H_
#define LIGHTSPEED_BASE_LINUX_NETSTREAMSOURCE_H_
#include "../streams/netio_ifc.h"
#include "netStream.h"
#include "../containers/optional.h"
#include "../memory/staticAlloc.h"


namespace LightSpeed {



class LinuxNetAccept: public NetworkResourceCommon<INetworkStreamSource>,
						public INetworkSocket {
public:
	LinuxNetAccept(PNetworkAddress addr,
			natural count ,natural timeout,
			natural streamDefTimeout);

	~LinuxNetAccept();

	virtual bool hasItems() const ;
	virtual PNetworkStream getNext() ;
	virtual PNetworkAddress getPeerAddr() const ;
	virtual PNetworkAddress getLocalAddr() const ;

	using INetworkStreamSource::wait;
	virtual integer getSocket(int idx) const {return idx?-1:acceptSock;}
	virtual natural getDefaultWait() const {return waitForInput;}

protected:

	int acceptSock;
	mutable PNetworkAddress localAddr;
	PNetworkAddress remoteAddr;
	natural count;
	natural streamDefTimeout;

	virtual natural doWait(natural waitFor, natural timeout) const;

};

class LinuxNetConnect: public NetworkResourceCommon<INetworkStreamSource>,
					   public INetworkSocket {
public:

	LinuxNetConnect(PNetworkAddress addr,
			natural count ,natural timeout,
			natural streamDefTimeout);
	virtual ~LinuxNetConnect();

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
	mutable int readySocket;

	virtual natural doWait(natural waitFor, natural timeout) const;


	static const natural maxWatchSockets = 4;
	mutable AutoArray<int, StaticAlloc<maxWatchSockets> > asyncSocket;

	void setupSocket() const;

	PNetworkStream makeConnect();
};

}

#endif /* LIGHTSPEED_BASE_LINUX_NETSTREAMSOURCE_H_ */
