/*
 * netService.cpp
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#include "netService.h"
#include "../text/textFormat.tcc"
#include "../memory/staticAlloc.h"
#include <netdb.h>
#include "../exceptions/netExceptions.h"
#include "netAddress.h"
#include "networkEventListener.h"
#include "netStreamSource.h"
#include <string.h>
#include "netDgramSource.h"
#include <stdlib.h>
#include "../text/textParser.tcc"
#include "linuxNetWaitingObj.h"
#include "../exceptions/outofmemory.h"

namespace LightSpeed {



PNetworkAddress LinuxNetService::createAddr(ConstStrA remoteAddr, natural Port)
{
	TextParser<char, StaticAlloc<256> > fmt;
	if (fmt("[%1]:%2",remoteAddr)) {
		ConstStrA addr = fmt[1];
		ConstStrA port = fmt[2];
		return createAddr(addr,port);
	} else if (fmt("%1:%2",remoteAddr)) {
		ConstStrA addr = fmt[1];
		ConstStrA port = fmt[2];
		return createAddr(addr,port);
	} else {
		TextFormatBuff<char, StaticAlloc<50> > fmt;
		fmt("%1") << Port;
		return createAddr(remoteAddr,ConstStrA(fmt.write()));
	}
}

PNetworkAddress LinuxNetService::createAddr(ConstStrA remoteAddr,ConstStrA service) {

	StringA a = remoteAddr;
	StringA b = service;
	const char *node = a.empty()?0:a.c_str();
	const char *svc = b.empty()?0:b.c_str();

	struct addrinfo *result = 0;
	struct addrinfo filter;
	memset(&filter,0,sizeof(filter));
	filter.ai_family = AF_UNSPEC;
	filter.ai_socktype = SOCK_STREAM;
	if (node == 0) filter.ai_flags = AI_PASSIVE;


	int res = getaddrinfo(node,svc,&filter,&result);
	if (res != 0) {
		throw NetworkResolveError(THISLOCATION,errno,String(a) + ConstStrW(L":") + String(b));
	}
	return new LinuxNetAddress(result,&freeaddrinfo);

}

static void localfreeaddrinfo(struct addrinfo *a) {
	free(a);
}


PNetworkAddress LinuxNetService::createAddr(const void *addr, natural len)
{
	struct addrinfo *nfo = (struct addrinfo *)malloc(sizeof(addrinfo) + len);
	if (nfo == 0) throw OutOfMemoryException(THISLOCATION);
	memcpy(nfo+1,addr,len);
	nfo->ai_addr = reinterpret_cast<struct sockaddr *>(nfo+1);
	nfo->ai_addrlen = len;
	nfo->ai_canonname = 0;
	nfo->ai_family = nfo->ai_addr->sa_family;
	nfo->ai_flags = 0;
	nfo->ai_next = 0;
	nfo->ai_protocol = IPPROTO_UDP;
	nfo->ai_socktype = SOCK_STREAM;
	try {
		PNetworkAddress assignedAddr = new LinuxNetAddress(nfo,&localfreeaddrinfo);
		return assignedAddr;
	} catch (...) {
		free(nfo);
		throw;
	}
}

static LinuxNetService myservices;
static INetworkServices *globServices = &myservices;


INetworkServices & INetworkServices::getNetServices()
{
	return *globServices;
}




void INetworkServices::setIOServices(INetworkServices *newServices)
{
	globServices = newServices;
}



PNetworkEventListener LinuxNetService::createEventListener()
{
	LinuxNetworkEventListener *x = new LinuxNetworkEventListener;
	x->enableMTAccess();
	return x;
}



PNetworkStreamSource LinuxNetService::createStreamSource(PNetworkAddress address,StreamOpenMode::Type  mode, natural count, natural timeout, natural defTm)
{
	LinuxNetAddress *addr = dynamic_cast<LinuxNetAddress *>(address.get());
	if (addr == 0) throw NetworkInvalidAddressException(THISLOCATION,address);
	if (mode == StreamOpenMode::passive || (mode == StreamOpenMode::useAddress
					&& (addr->getAddrInfo()->ai_flags & AI_PASSIVE))) {
		return new LinuxNetAccept(address,count,timeout,defTm);
	} else {
		return new LinuxNetConnect(address,count,timeout,defTm);
	}
}

PNetworkDatagramSource LinuxNetService::createDatagramSource(
				natural port, natural timeout ) {

	return new LinuxNetDgamSource(port,timeout,1);

}

PNetworkWaitingObject LinuxNetService::createWaitingObject() {
	return new LinuxNetWaitingObj;
}

PNetworkAddress LinuxNetService::createAddr(ConstStrA remoteAddr, natural Port,
		IPVersion version) {
	TextParser<char, StaticAlloc<256> > fmt;
	if (fmt("[%1]:%2",remoteAddr)) {
		ConstStrA addr = fmt[1];
		ConstStrA port = fmt[2];
		return createAddr(addr,port,version);
	} else if (fmt("%1:%2",remoteAddr)) {
		ConstStrA addr = fmt[1];
		ConstStrA port = fmt[2];
		return createAddr(addr,port,version);
	} else {
		TextFormatBuff<char, StaticAlloc<50> > fmt;
		fmt("%1") << Port;
		return createAddr(remoteAddr,ConstStrA(fmt.write()),version);
	}
}

PNetworkAddress LinuxNetService::createAddr(ConstStrA remoteAddr,
		ConstStrA service, IPVersion version) {
	StringA a = remoteAddr;
	StringA b = service;
	const char *node = a.empty()?0:a.c_str();
	const char *svc = b.empty()?0:b.c_str();

	struct addrinfo *result = 0;
	struct addrinfo filter;
	memset(&filter,0,sizeof(filter));
	switch (version) {
	case ipVer4: filter.ai_family = AF_INET;break;
	case ipVer6: filter.ai_family = AF_INET6;break;
	default:
	case ipVerAny: filter.ai_family = AF_UNSPEC;break;
	}

	filter.ai_socktype = SOCK_STREAM;
	if (node == 0) filter.ai_flags = AI_PASSIVE;


	int res = getaddrinfo(node,svc,&filter,&result);
	if (res != 0) {
		throw NetworkResolveError(THISLOCATION,errno,String(a) + ConstStrW(L":") + String(b));
	}
	return new LinuxNetAddress(result,&freeaddrinfo);
}

LinuxNetService::IPVersion LinuxNetService::getAvailableVersions() const {
	return ipVerAny;
}

ConstStrA LinuxNetService::getLoopbackAddr(IPVersion ) const {
	return "localhost";
}

}
