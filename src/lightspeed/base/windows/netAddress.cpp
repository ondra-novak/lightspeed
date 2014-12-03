/*
 * netAddress.cpp
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include "netAddress.h"
#include "../text/textFormat.tcc"
#include "../memory/smallAlloc.h"
#include <string.h>
#include "../text/textParser.tcc"
#include "../exceptions/netExceptions.h"
#include "../exceptions/outofmemory.h"
#include "../debug/dbglog.h"

namespace LightSpeed {

	static ConstStrA localhost("0");

	ConstStrA INetworkAddress::getLocalhost() {return localhost;}

///	ConstStrA INetworkAddress::localhost("xxx");
	

WindowsNetAddress::WindowsNetAddress(struct addrinfo *addrinfo,
		addrinfo_destructor destructor)
		:addrinfo(addrinfo),destructor(destructor)
		,reuseAddr(false) {
	

}

WindowsNetAddress::~WindowsNetAddress() {
	destructor(addrinfo);
}


StringA WindowsNetAddress::asString(bool resolve) {
	if (addrinfo == 0) return StringA();
	TextFormatBuff<char, SmallAlloc<256> > buff;
	char hostbuff[512];
	char svcbuff[256];

	int flags = resolve?0:NI_NUMERICHOST|NI_NUMERICSERV;

	getnameinfo(addrinfo->ai_addr, addrinfo->ai_addrlen,
			hostbuff,sizeof(hostbuff),
			svcbuff,sizeof(svcbuff),flags);

	buff("%1:%2") << hostbuff << svcbuff;
	return StringA(buff.write());

}
bool WindowsNetAddress::equalTo(const INetworkAddress &other) const {
	const WindowsNetAddress *o = dynamic_cast<const WindowsNetAddress *>(&other);

	if (o->addrinfo == 0 && addrinfo == 0) return true;
	if (o->addrinfo == 0 || addrinfo == 0) return false;
	if (o->addrinfo == addrinfo) return true;
	if (o->addrinfo->ai_addrlen == addrinfo->ai_addrlen)
		return memcmp(o->addrinfo->ai_addr,addrinfo->ai_addr,o->addrinfo->ai_addrlen) == 0;
	else
		return false;

}



natural WindowsNetAddress::getSockAddress(void *buffer, natural size) const {
	if (buffer == 0) return addrinfo->ai_addrlen;
	if (addrinfo->ai_addrlen < size) {
		return getSockAddress(buffer,addrinfo->ai_addrlen);
	}
	memcpy(buffer,addrinfo->ai_addr,size);
	return size;
}

static void WINAPI localfreeaddrinfo(PADDRINFOA a) {
	free(a);
}


WindowsNetAddress * WindowsNetAddress::createAddr( 
		ConstStrA remoteAddr, natural Port,
		INetworkServicesIP::IPVersion version)
{
	TextParser<char, StaticAlloc<256> > fmt;
	if (fmt("[%1]:%2",remoteAddr)) {
		ConstStrA addr = fmt[1];
		ConstStrA port = fmt[2];
		return createAddr(addr,port,version);
	} else if (fmt("%[0-9.a-zA-Z]1:%[0-9]2",remoteAddr)) {
		ConstStrA addr = fmt[1];
		ConstStrA port = fmt[2];
		return createAddr(addr,port,version);
	} else {
		TextFormatBuff<char, StaticAlloc<50> > fmt;
		fmt("%1") << Port;
		return createAddr(remoteAddr,ConstStrA(fmt.write()),version);
	}
}

WindowsNetAddress * WindowsNetAddress::createAddr( 
				ConstStrA remoteAddr,ConstStrA service,
				INetworkServicesIP::IPVersion version)
{
	StringA a = remoteAddr;
	StringA b = service;	
	const char *node = a.empty()?0:a.c_str();
	const char *svc = b.empty()?0:b.c_str();

	struct addrinfo *result = 0;
	struct addrinfo filter;
	memset(&filter,0,sizeof(filter));
	filter.ai_family = AF_UNSPEC;
	filter.ai_socktype = 0;
	if (node == 0) filter.ai_flags  |= AI_PASSIVE;

	switch (version) {
		case INetworkServicesIP::ipVerAny:
			filter.ai_family = AF_UNSPEC;
			filter.ai_flags |= AI_ALL;
			break;
		case INetworkServicesIP::ipVer6:
			filter.ai_family = AF_INET6;
			break;
		case INetworkServicesIP::ipVer4:
			filter.ai_family = AF_INET;
			break;
	}


	int res = getaddrinfo(node,svc,&filter,&result);
	if (res != 0) {
		throw NetworkResolveError(THISLOCATION,res,String(a) + ConstStrW(L":") + String(b));
	}
	result->ai_flags = filter.ai_flags;
	if (version == INetworkServicesIP::ipVerAny 
		&& result->ai_addr->sa_family == AF_INET6) {
			result->ai_flags |= AI_V4MAPPED;
	}

	return new WindowsNetAddress(result,&freeaddrinfo);

}

WindowsNetAddress * WindowsNetAddress::createAddr( const void *addr, natural len )
{
	struct addrinfo *nfo = (struct addrinfo *)malloc(sizeof(struct addrinfo) + len);
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
		return new WindowsNetAddress(nfo,&localfreeaddrinfo);
	} catch (...) {
		free(nfo);
		throw;
	}
}

bool WindowsNetAddress::enableReuseAddr( bool enable )
{
	reuseAddr = enable;
	return true;
}

LightSpeed::natural WindowsNetAddress::getPortNumber() const
{
	if (addrinfo == 0) return naturalNull;
	if (addrinfo->ai_addr->sa_family == AF_INET) {
		const SOCKADDR_IN *a = reinterpret_cast<const SOCKADDR_IN *>(addrinfo->ai_addr);
		return htons(a->sin_port);
	} else if (addrinfo->ai_addr->sa_family == AF_INET6) {
		const SOCKADDR_IN6 *a = reinterpret_cast<const SOCKADDR_IN6 *>(addrinfo->ai_addr);
		return htons(a->sin6_port);
	} else {
		return naturalNull;
	}
}

bool WindowsNetAddress::lessThan( const INetworkAddress &other ) const
{
	const WindowsNetAddress *x = dynamic_cast<const WindowsNetAddress *>(&other);
	if (x == 0) {
		return typeid(*this).before(typeid(other)) != 0;
	} else {
		if (addrinfo==0) return x->addrinfo != 0;
		if (x->addrinfo == 0) return false;
		if (addrinfo->ai_family != x->addrinfo->ai_family)
			return addrinfo->ai_family < x->addrinfo->ai_family;
		if (addrinfo->ai_addrlen != x->addrinfo->ai_addrlen)
			return addrinfo->ai_addrlen != x->addrinfo->ai_addrlen;
		if (addrinfo->ai_family == AF_INET) {
			struct sockaddr_in *sin = reinterpret_cast<struct sockaddr_in *>(addrinfo->ai_addr);
			struct sockaddr_in *osin = reinterpret_cast<struct sockaddr_in *>(x->addrinfo->ai_addr);
			if (sin->sin_addr.s_addr != osin->sin_addr.s_addr)
				return sin->sin_addr.s_addr < osin->sin_addr.s_addr;
			return sin->sin_port < osin->sin_port;
		} else if (addrinfo->ai_family == AF_INET6) {
			struct sockaddr_in6 *sin = reinterpret_cast<struct sockaddr_in6 *>(addrinfo->ai_addr);
			struct sockaddr_in6 *osin = reinterpret_cast<struct sockaddr_in6 *>(x->addrinfo->ai_addr);
			int cmp = memcmp(&sin->sin6_addr,&osin->sin6_addr,16);
			if (cmp !=0 ) return cmp < 0;
			return sin->sin6_port < osin->sin6_port;
		} else {
			int cmp = memcmp(addrinfo->ai_addr,x->addrinfo->ai_addr,addrinfo->ai_addrlen);
			return cmp < 0;
		}
	}
	
}

}
