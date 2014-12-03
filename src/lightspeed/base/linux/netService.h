/*
 * netService.h
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_LINUX_NETSERVICE_H_
#define LIGHTSPEED_BASE_LINUX_NETSERVICE_H_
#include "../streams/netio_ifc.h"


struct sockaddr;

namespace LightSpeed {

class LinuxNetService: public INetworkServices, public INetworkServicesIP {
public:


	///Creates network stream source
	virtual PNetworkStreamSource createStreamSource(
			PNetworkAddress address,
			StreamOpenMode::Type  mode,
			natural count = 1,
			natural timeout = naturalNull,
			natural streamDefTimeout = naturalNull);



	virtual PNetworkAddress createAddr(ConstStrA remoteAddr,natural port);

	virtual PNetworkAddress createAddr(ConstStrA remoteAddr,ConstStrA service);

	virtual PNetworkAddress createAddr(const void *addr, natural len);

	virtual PNetworkEventListener createEventListener();

	virtual PNetworkDatagramSource createDatagramSource(
					natural port = 0, natural timeout = naturalNull);
	virtual PNetworkWaitingObject createWaitingObject();

	virtual PNetworkAddress createAddr(ConstStrA remoteAddr,
		natural Port, IPVersion version);
	virtual PNetworkAddress createAddr(ConstStrA remoteAddr,
		ConstStrA service, IPVersion version) ;


	virtual IPVersion getAvailableVersions() const ;


	virtual ConstStrA getLoopbackAddr(IPVersion ver = ipVerAny) const ;



};

}

#endif /* LIGHTSPEED_BASE_LINUX_NETSERVICE_H_ */
