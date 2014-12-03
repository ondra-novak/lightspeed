/*
 * netAddress.h
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_LINUX_NETADDRESS_H_
#define LIGHTSPEED_BASE_LINUX_NETADDRESS_H_

#include "../streams/netio_ifc.h"
#include <netdb.h>

namespace LightSpeed {

class LinuxNetAddress: public INetworkAddress, public INetworkAddrEx {
public:
	typedef void (*addrinfo_destructor)(struct addrinfo *__ai);

	LinuxNetAddress(struct addrinfo *addrinfo, addrinfo_destructor destructor);
	virtual ~LinuxNetAddress();



	virtual StringA asString(bool resolve = false) ;

	virtual bool equalTo(const INetworkAddress &other) const ;

	virtual bool lessThan(const INetworkAddress &other) const ;


	struct addrinfo *getAddrInfo() const {return addrinfo;}

	virtual natural getSockAddress(void *buffer, natural size) const;

	virtual bool enableReuseAddr(bool enable);
	virtual natural getPortNumber() const;

protected:

	LinuxNetAddress(const LinuxNetAddress &other);

	struct addrinfo *addrinfo;
	addrinfo_destructor destructor;

};

}



#endif /* LIGHTSPEED_BASE_LINUX_NETADDRESS_H_ */
