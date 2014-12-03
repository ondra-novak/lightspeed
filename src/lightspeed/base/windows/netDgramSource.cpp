/*
 * netDgramSource.cpp
 *
 *  Created on: 7.7.2011
 *      Author: ondra
 */

#include <string.h>
#include <stdlib.h>
#include "netDgramSource.h"
#include "../streams/memfile.tcc"
#include "../exceptions/netExceptions.h"
#include "netAddress.h"
#include <algorithm>

namespace LightSpeed {



WindowsNetDgamSource::WindowsNetDgamSource(natural port, natural timeout, natural startDID)
	:sres(createDatagramSocket(port),timeout),dgrId(startDID)
{

}

natural WindowsNetDgamSource::wait(natural waitFor, natural timeout) const {
	return sres.userWait(waitFor,timeout);
}
 void WindowsNetDgamSource::setWaitHandler(IWaitHandler *handler) {
	sres.setWaitHandler(handler);
}

void WindowsNetDgamSource::setTimeout(natural time_in_ms) {
	sres.setTimeout(time_in_ms);
}

natural WindowsNetDgamSource::getTimeout() const {
	return sres.getTimeout();
}

PNetworkDatagram WindowsNetDgamSource::receive() {

	sres.wait();

	//TODO: optimize memory alocation
	RefCntPtr<WindowsNetDatagram> dgr = new WindowsNetDatagram(this);

	struct sockaddr *saddr = reinterpret_cast<struct sockaddr *>(dgr->addrbuff);
	socklen_t len = sizeof(dgr->addrbuff);

	u_long res = 0;
	ioctlsocket(sres.sock,FIONREAD,&res);
	dgr->inputData.setSize(res);
	char *buff = reinterpret_cast<char *>(dgr->inputData.getBuffer().data());

	int r = ::recvfrom(sres.sock,buff,res,0,saddr,&len);
	if (r == -1) {
		int err = WSAGetLastError(); throw NetworkIOError(THISLOCATION,err,"recvfrom failed");
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

PNetworkDatagram WindowsNetDgamSource::create() {
	return new(this) WindowsNetDatagram(this);

}

PNetworkDatagram WindowsNetDgamSource :: create(PNetworkAddress adr) {
	RefCntPtr<WindowsNetDatagram> dgr = new(this) WindowsNetDatagram(this);
	WindowsNetAddress *a = dynamic_cast<WindowsNetAddress *>(adr.get());
	if (a == 0 || a->getAddrInfo()== 0
			|| a->getAddrInfo()->ai_addrlen > sizeof(dgr->addrbuff))
		throw NetworkInvalidAddressException(THISLOCATION,adr);
	dgr->assignedAddr = a;
	memcpy(dgr->addrbuff,a->getAddrInfo()->ai_addr,a->getAddrInfo()->ai_addrlen);
	dgr->addrlen = a->getAddrInfo()->ai_addrlen;

	return dgr.get();

}

integer WindowsNetDgamSource ::getSocket(int index) const {
	return (integer)(index?-1:sres.sock);
}

static void localfreeaddrinfo(struct addrinfo *a) {
	free(a);
}

PNetworkAddress WindowsNetDatagram::getTarget(){
	if (assignedAddr == 0) {
		if (addrlen == 0)
			return nil;
		else {
			
			return WindowsNetAddress::createAddr(addrbuff,addrlen);
/*
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
				assignedAddr = new WindowsNetAddress(nfo,&localfreeaddrinfo);
			} catch (...) {
				free(nfo);
				throw;
			}*/
		}
	}
	return assignedAddr;
}
void WindowsNetDatagram::send() {
	if (addrlen == 0)
		throw NetworkInvalidAddressException(THISLOCATION,nil);
	else {
		int r = (int)::sendto(owner->sres.sock,
			reinterpret_cast<const char *>(outputData.getBuffer().data()),
				(int)outputData.getBuffer().length(),0,
				(struct sockaddr *)addrbuff, (int)addrlen);
		if (r == -1) {
			int err = WSAGetLastError();
			throw NetworkIOError(THISLOCATION, err, "Cannot send datagram");
		}
		resetTarget();

	}
}
void WindowsNetDatagram::sendTo(PNetworkAddress target) {
	if (target == assignedAddr) send();
	else {
		WindowsNetAddress *a = dynamic_cast<WindowsNetAddress *>(target.get());
		if (a == 0)
			throw NetworkInvalidAddressException(THISLOCATION,target);
		int r = ::sendto(owner->sres.sock,
				reinterpret_cast<const char *>(outputData.getBuffer().data()),
				(int)outputData.getBuffer().length(),0,
				a->getAddrInfo()->ai_addr, (int)a->getAddrInfo()->ai_addrlen);
		if (r == -1) {
			int err = WSAGetLastError();
			throw NetworkIOError(THISLOCATION, err, "Cannot send datagram");
		}
		resetTarget();
	}
}
void WindowsNetDatagram::resetTarget() {
	assignedAddr = nil;
	addrlen = 0;
}
void WindowsNetDatagram::rewind() {
	inputData.setPos(0);
}
void WindowsNetDatagram::clear() {
	outputData.clear();
}
natural WindowsNetDatagram::getUID() const {
	while (dgrid == naturalNull) {
		dgrid = const_cast<WindowsNetDgamSource *>(owner)->dgrId++;
	}
	return dgrid;
}
natural WindowsNetDatagram::read(void *buffer,  natural size) {
	return inputData.read(buffer,size);
}
natural WindowsNetDatagram::write(const void *buffer,  natural size) {
	return outputData.write(buffer,size);
}
natural WindowsNetDatagram::peek(void *buffer, natural size) const {
	return inputData.peek(buffer,size);
}
bool WindowsNetDatagram::canRead() const {
	return inputData.canRead();
}
bool WindowsNetDatagram::canWrite() const {
	return true;
}
void WindowsNetDatagram::flush() {

}
natural WindowsNetDatagram::dataReady() const {
	return inputData.dataReady();
}
natural WindowsNetDatagram::read(void *buffer,  natural size, FileOffset offset) const {
	return inputData.read(buffer,size,offset);
}
natural WindowsNetDatagram::write(const void *buffer,  natural size, FileOffset offset) {
	return outputData.write(buffer,size,offset);
}
void WindowsNetDatagram::setSize(FileOffset size) {
	outputData.setSize(size);
}
WindowsNetDatagram::FileOffset WindowsNetDatagram::size() const {
	return inputData.size();
}
bool WindowsNetDatagram::checkAddress(PNetworkAddress address) {
	WindowsNetAddress *a = dynamic_cast<WindowsNetAddress *>(address.get());
	if (a == 0 || a->getAddrInfo() == 0
		|| a->getAddrInfo()->ai_addrlen != addrlen) return false;
	return memcmp(addrbuff,&a->getAddrInfo()->ai_addrlen,addrlen) == 0;



}
WindowsNetDatagram::~WindowsNetDatagram() {
	/* Note: removed - datagram must be sent manually
	if (!std::uncaught_exception()
		&& addrlen != 0 && !outputData.empty()) {
			send();
	}
	*/
}

void WindowsNetDatagram::closeOutput()
{
	send();
}

ConstStringT<byte> WindowsNetDatagram::peekOutputBuffer() const
{
	return outputData.getBuffer();

}



UINT_PTR WindowsNetDgamSource::createDatagramSocket(natural port) {


	SOCKET s;
	if (port) {
		char portName[50];
		sprintf_s(portName,"%d",port);

		const char *svc = portName;

		struct addrinfo *result = 0;
		struct addrinfo filter;
		memset(&filter,0,sizeof(filter));
		filter.ai_family = AF_INET6;
		filter.ai_socktype = SOCK_DGRAM;
		filter.ai_flags = AI_PASSIVE;


		int res = getaddrinfo(0,svc,&filter,&result);
		if (res != 0) {
			throw NetworkResolveError(THISLOCATION,WSAGetLastError(),ConstStrW(L":") + String(portName));
		}
		WindowsNetAddress addr(result,&freeaddrinfo);


		struct addrinfo *ainfo = addr.getAddrInfo();

		s = socket(ainfo->ai_family, SOCK_DGRAM, ainfo->ai_protocol);
		if (s == -1) throw NetworkPortOpenException(THISLOCATION,WSAGetLastError(),port);


		int e = bind(s,ainfo->ai_addr,int(ainfo->ai_addrlen));

		if (e != 0)
			throw NetworkPortOpenException(THISLOCATION,WSAGetLastError(),port);
	} else {

		s = socket(AF_INET6, SOCK_DGRAM, 0);
		if (s == -1) throw NetworkPortOpenException(THISLOCATION,WSAGetLastError(),port);

	}
	u_long nonblk = 1;
	ioctlsocket(s,FIONBIO,&nonblk);

	int on = 0;

	setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY,(char *)&on, sizeof(on));
	return s;


}

}
