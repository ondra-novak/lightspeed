/*
 * netService.h
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_WINDOWS_NETSERVICE_H_
#define LIGHTSPEED_BASE_WINDOWS_NETSERVICE_H_
#include "../streams/netio_ifc.h"
#include <winsock2.h>


struct sockaddr;

namespace LightSpeed {

class WindowsNetService: public INetworkServices, 
						 public INetworkServicesIP,
						 public INamedPipeServices{
public:
	WindowsNetService();


	///Creates network stream source
	virtual PNetworkStreamSource createStreamSource(
			PNetworkAddress address,
			StreamOpenMode::Type mode,
			natural count = 1,
			natural timeout = naturalNull,
			natural streamDefTimeout = naturalNull);



	virtual PNetworkAddress createAddr(ConstStrA remoteAddr,natural port);

	virtual PNetworkAddress createAddr(ConstStrA remoteAddr,ConstStrA service);

	virtual PNetworkAddress createAddr(const void *addr, natural len);

	virtual PNetworkAddress createAddr(ConstStrA remoteAddr, 
		natural Port, IPVersion version);
	virtual PNetworkAddress createAddr(ConstStrA remoteAddr,
		ConstStrA service, IPVersion version);

	virtual PNetworkEventListener createEventListener();

	virtual PNetworkDatagramSource createDatagramSource(
					natural port = 0, natural timeout = naturalNull);

	virtual IPVersion getAvailableVersions() const;
	virtual ConstStrA getLoopbackAddr(IPVersion ver) const;
	virtual PNetworkWaitingObject createWaitingObject();

	virtual PNetworkStreamSource createNamedPipe( bool server, ConstStrW pipeName, PipeMode mode, 
		natural maxInstances = naturalNull, ConstStrA securityDesc = ConstStrA( ) , 
		natural connectTimeout = naturalNull, natural streamDefTimeout = naturalNull );


	struct InitWinSock: public WSADATA {
	public:
		InitWinSock();

		~InitWinSock();
	};

	InitWinSock wsaData;

	bool useIPv4;
};

}

#endif /* LIGHTSPEED_BASE_WINDOWS_NETSERVICE_H_ */
