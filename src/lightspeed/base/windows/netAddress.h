/*
 * netAddress.h
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_WINDOWS_NETADDRESS_H_
#define LIGHTSPEED_BASE_WINDOWS_NETADDRESS_H_

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include "../streams/netio_ifc.h"

namespace LightSpeed {

class WindowsNetAddress: public INetworkAddress, public INetworkAddrEx {
public:
	typedef void (WINAPI  *addrinfo_destructor)(PADDRINFOA __ai);

	WindowsNetAddress(struct addrinfo *addrinfo, 
					addrinfo_destructor destructor);
	virtual ~WindowsNetAddress();



	virtual StringA asString(bool resolve = false) ;

	virtual bool equalTo(const INetworkAddress &other) const ;


	struct addrinfo *getAddrInfo() const {return addrinfo;}

	virtual natural getSockAddress(void *buffer, natural size) const;
	virtual bool enableReuseAddr(bool enable);
	bool isReuseAddrEnabled() const {return reuseAddr;}

	static WindowsNetAddress *createAddr(
				ConstStrA remoteAddr, natural Port, 
				INetworkServicesIP::IPVersion version);
	static WindowsNetAddress *createAddr(
				ConstStrA remoteAddr,ConstStrA service,
				INetworkServicesIP::IPVersion version);
	static WindowsNetAddress *createAddr(const void *addr, natural len);

	virtual natural getPortNumber() const;
	
	virtual bool lessThan(const INetworkAddress &other) const;


protected:

	WindowsNetAddress(const WindowsNetAddress &other);

	struct addrinfo *addrinfo;
	addrinfo_destructor destructor;
	bool reuseAddr;

};

}



#endif /* LIGHTSPEED_BASE_WINDOWS_NETADDRESS_H_ */
