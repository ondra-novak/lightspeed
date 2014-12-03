/*
 * netDgramSource.cpp
 *
 *  Created on: 7.7.2011
 *      Author: ondra
 */

#include "netDgramSource.h"
#include <unistd.h>
#include <sys/socket.h>
#include "../streams/memfile.tcc"
#include <sys/ioctl.h>
#include "../exceptions/netExceptions.h"
#include <errno.h>
#include "netAddress.h"
#include <string.h>
#include <stdlib.h>
#include "../exceptions/outofmemory.h"

namespace LightSpeed {



ConstStringT<byte> LinuxNetDatagram::peekOutputBuffer() const {
	return outputData.getBuffer();
}

LinuxNetDgamSource::LinuxNetDgamSource(natural port, natural timeout, natural startDID)
	:sres(createDatagramSocket(port),timeout),dgrId(startDID)
{

}

natural LinuxNetDgamSource::wait(natural waitFor, natural timeout) const {
	return sres.userWait(waitFor,timeout);
}
 void LinuxNetDgamSource::setWaitHandler(IWaitHandler *handler) {
	sres.setWaitHandler(handler);
}

void LinuxNetDgamSource::setTimeout(natural time_in_ms) {
	sres.setTimeout(time_in_ms);
}

natural LinuxNetDgamSource::getTimeout() const {
	return sres.getTimeout();
}

PNetworkDatagram LinuxNetDgamSource::receive() {

	sres.wait();

	RefCntPtr<LinuxNetDatagram> dgr = new(dgmpool) LinuxNetDatagram(this);

	struct sockaddr *saddr = reinterpret_cast<struct sockaddr *>(dgr->addrbuff);
	socklen_t len = sizeof(dgr->addrbuff);

	u_long res = 0;
	ioctl(sres.sock,FIONREAD,&res);
	dgr->inputData.setSize(res);
	void *buff = dgr->inputData.getBuffer().data();

	int r = ::recvfrom(sres.sock,buff,res,0,saddr,&len);
	if (r == -1) {
		int err = errno; throw NetworkIOError(THISLOCATION,err,"recvfrom failed");
	}

	dgr->addrlen = len;
/*
	dgr->addrInfo.ai_addr = saddr;
	dgr->addrInfo.ai_addrlen = len;
	dgr->addrInfo.ai_canonname = 0;
	dgr->addrInfo.ai_family = saddr->sa_family;
	dgr->addrInfo.ai_flags = 0;
	dgr->addrInfo.ai_next = 0;
	dgr->addrInfo.ai_protocol = IPPROTO_UDP;
	dgr->addrInfo.ai_socktype = SOCK_STREAM;
*/

	return dgr.get();
}

PNetworkDatagram LinuxNetDgamSource::create() {
	return new(dgmpool) LinuxNetDatagram(this);

}

PNetworkDatagram LinuxNetDgamSource :: create(PNetworkAddress adr) {
	RefCntPtr<LinuxNetDatagram> dgr = new(dgmpool) LinuxNetDatagram(this);
	LinuxNetAddress *a = dynamic_cast<LinuxNetAddress *>(adr.get());
	if (a == 0 || a->getAddrInfo()== 0
			|| a->getAddrInfo()->ai_addrlen > sizeof(dgr->addrbuff))
		throw NetworkInvalidAddressException(THISLOCATION,adr);
	dgr->assignedAddr = a;
	memcpy(dgr->addrbuff,a->getAddrInfo()->ai_addr,a->getAddrInfo()->ai_addrlen);
	dgr->addrlen = a->getAddrInfo()->ai_addrlen;

	return dgr.get();

}


integer LinuxNetDgamSource ::getSocket(int index) const {
	return index?-1:sres.sock;
}

static void localfreeaddrinfo(struct addrinfo *a) {
	free(a);
}

PNetworkAddress LinuxNetDatagram::getTarget(){
	if (assignedAddr == 0) {
		if (addrlen == 0)
			return nil;
		else {

			struct addrinfo *nfo = (struct addrinfo *)malloc(sizeof(addrinfo) + addrlen);
			if (nfo == 0) throw OutOfMemoryException(THISLOCATION);
			memcpy(nfo+1,addrbuff,addrlen);
			nfo->ai_addr = reinterpret_cast<struct sockaddr *>(nfo+1);
			nfo->ai_addrlen = addrlen;
			nfo->ai_canonname = 0;
			nfo->ai_family = nfo->ai_addr->sa_family;
			nfo->ai_flags = 0;
			nfo->ai_next = 0;
			nfo->ai_protocol = IPPROTO_UDP;
			nfo->ai_socktype = SOCK_STREAM;
			try {

				class DynLinuxNetAddr: public LinuxNetAddress, public DynObject {
				public:
					DynLinuxNetAddr(struct addrinfo *addrinfo, addrinfo_destructor destructor)
						:LinuxNetAddress(addrinfo,destructor) {}
				};

				assignedAddr = new(owner->dgmpool) DynLinuxNetAddr(nfo,&localfreeaddrinfo);
			} catch (...) {
				free(nfo);
				throw;
			}
		}
	}
	return assignedAddr;
}
void LinuxNetDatagram::send() {
	if (addrlen == 0)
		throw NetworkInvalidAddressException(THISLOCATION,nil);
	else {
		int r = ::sendto(owner->sres.sock,outputData.getBuffer().data(),
				outputData.getBuffer().length(),0,
				(struct sockaddr *)addrbuff, addrlen);
		if (r == -1) {
			int err = errno;
			throw NetworkIOError(THISLOCATION, err, "Cannot send datagram");
		}
		resetTarget();

	}
}
void LinuxNetDatagram::sendTo(PNetworkAddress target) {
	if (target == assignedAddr) send();
	else {
		LinuxNetAddress *a = dynamic_cast<LinuxNetAddress *>(target.get());
		if (a == 0)
			throw NetworkInvalidAddressException(THISLOCATION,target);
		int r = ::sendto(owner->sres.sock,outputData.getBuffer().data(),
				outputData.getBuffer().length(),0,
				a->getAddrInfo()->ai_addr, a->getAddrInfo()->ai_addrlen);
		if (r == -1) {
			int err = errno;
			throw NetworkIOError(THISLOCATION, err, "Cannot send datagram");
		}
		resetTarget();
	}
}
void LinuxNetDatagram::resetTarget() {
	assignedAddr = nil;
	addrlen = 0;
}
void LinuxNetDatagram::rewind() {
	inputData.setPos(0);
}
void LinuxNetDatagram::clear() {
	outputData.clear();
}
natural LinuxNetDatagram::getUID() const {
	while (dgrid == naturalNull) {
		dgrid = const_cast<LinuxNetDgamSource *>(owner)->dgrId++;
	}
	return dgrid;
}
natural LinuxNetDatagram::read(void *buffer,  natural size) {
	return inputData.read(buffer,size);
}
natural LinuxNetDatagram::write(const void *buffer,  natural size) {
	return outputData.write(buffer,size);
}
natural LinuxNetDatagram::peek(void *buffer, natural size) const {
	return inputData.peek(buffer,size);
}
bool LinuxNetDatagram::canRead() const {
	return inputData.canRead();
}
bool LinuxNetDatagram::canWrite() const {
	return true;
}
void LinuxNetDatagram::flush() {

}
natural LinuxNetDatagram::dataReady() const {
	return inputData.dataReady();
}
natural LinuxNetDatagram::read(void *buffer,  natural size, FileOffset offset) const {
	return inputData.read(buffer,size,offset);
}
natural LinuxNetDatagram::write(const void *buffer,  natural size, FileOffset offset) {
	return outputData.write(buffer,size,offset);
}
void LinuxNetDatagram::setSize(FileOffset size) {
	outputData.setSize(size);
}
LinuxNetDatagram::FileOffset LinuxNetDatagram::size() const {
	return inputData.size();
}
bool LinuxNetDatagram::checkAddress(PNetworkAddress address) {
	LinuxNetAddress *a = dynamic_cast<LinuxNetAddress *>(address.get());
	if (a == 0 || a->getAddrInfo() == 0
		|| a->getAddrInfo()->ai_addrlen != addrlen) return false;
	return memcmp(addrbuff,a->getAddrInfo()->ai_addr,addrlen) == 0;



}
LinuxNetDatagram::~LinuxNetDatagram() {
/* Note: removed - datagram must be sent manually

	if (!std::uncaught_exception()
		&& addrlen != 0 && !outputData.empty()) {
			send();
	}*/
}

int LinuxNetDgamSource::createDatagramSocket(natural port) {


	int s;
	if (port) {
		char portName[50];
		sprintf(portName,"%d",(int)port);

		const char *svc = portName;

		struct addrinfo *result = 0;
		struct addrinfo filter;
		memset(&filter,0,sizeof(filter));
		filter.ai_family = AF_INET6;
		filter.ai_socktype = SOCK_DGRAM;
		filter.ai_flags = AI_PASSIVE;


		int res = getaddrinfo(0,svc,&filter,&result);
		if (res != 0) {
			throw NetworkResolveError(THISLOCATION,errno,ConstStrW(L":") + String(portName));
		}
		LinuxNetAddress addr(result,&freeaddrinfo);


		struct addrinfo *ainfo = addr.getAddrInfo();

		s = socket(ainfo->ai_family, SOCK_DGRAM, ainfo->ai_protocol);
		if (s == -1) throw NetworkPortOpenException(THISLOCATION,errno,port);


		int e = bind(s,ainfo->ai_addr,ainfo->ai_addrlen);

		if (e != 0)
			throw NetworkPortOpenException(THISLOCATION,errno,port);
	} else {

		s = socket(AF_INET6, SOCK_DGRAM, 0);
		if (s == -1) throw NetworkPortOpenException(THISLOCATION,errno,port);

	}
	u_long nonblk = 1;
	ioctl(s,FIONBIO,&nonblk);

	int on = 0;

	setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY,(char *)&on, sizeof(on));
	return s;


}

}
