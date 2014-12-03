/*
 * netAddress.cpp
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#include "netAddress.h"
#include <netdb.h>
#include "../text/textFormat.tcc"
#include "../memory/smallAlloc.h"
#include <string.h>

namespace LightSpeed {

static ConstStrA localhost("0");

	ConstStrA INetworkAddress::getLocalhost() {return localhost;}

LinuxNetAddress::LinuxNetAddress(struct addrinfo *addrinfo,
		addrinfo_destructor destructor):addrinfo(addrinfo),destructor(destructor) {
	// TODO Auto-generated constructor stub

}

LinuxNetAddress::~LinuxNetAddress() {
	destructor(addrinfo);
}


StringA LinuxNetAddress::asString(bool resolve) {
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
bool LinuxNetAddress::equalTo(const INetworkAddress &other) const {
	const LinuxNetAddress *o = dynamic_cast<const LinuxNetAddress *>(&other);

	if (o->addrinfo == 0 && addrinfo == 0) return true;
	if (o->addrinfo == 0 || addrinfo == 0) return false;
	if (o->addrinfo == addrinfo) return true;
	if (o->addrinfo->ai_addrlen == addrinfo->ai_addrlen)
		return memcmp(o->addrinfo->ai_addr,addrinfo->ai_addr,o->addrinfo->ai_addrlen) == 0;
	else
		return false;

}



natural LinuxNetAddress::getSockAddress(void *buffer, natural size) const {
	if (buffer == 0) return addrinfo->ai_addrlen;
	if (addrinfo->ai_addrlen < size) {
		return getSockAddress(buffer,addrinfo->ai_addrlen);
	}
	memcpy(buffer,addrinfo->ai_addr,size);
	return size;
}

bool LinuxNetAddress::lessThan(const INetworkAddress& other) const {
	const LinuxNetAddress *x = dynamic_cast<const LinuxNetAddress *>(&other);
	if (x == 0) {
		return typeid(*this).before(typeid(other));
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
			int cmp = memcmp(&sin->sin6_addr.__in6_u,&osin->sin6_addr.__in6_u,16);
			if (cmp !=0 ) return cmp < 0;
			return sin->sin6_port < osin->sin6_port;
		} else {
			int cmp = memcmp(addrinfo->ai_addr,x->addrinfo->ai_addr,addrinfo->ai_addrlen);
			return cmp < 0;
		}
	}
}

bool LinuxNetAddress::enableReuseAddr(bool ) {
	return true;
}
natural LinuxNetAddress::getPortNumber() const {
	if (addrinfo != 0) {
		if (addrinfo->ai_addr->sa_family == AF_INET) {
			return htons(reinterpret_cast<struct sockaddr_in *>(addrinfo->ai_addr)->sin_port);
		} else if (addrinfo->ai_addr->sa_family == AF_INET6) {
			return htons(reinterpret_cast<struct sockaddr_in6 *>(addrinfo->ai_addr)->sin6_port);
		}
	}
	return naturalNull;
}

}
