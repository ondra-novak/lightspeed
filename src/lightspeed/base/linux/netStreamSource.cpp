/*
 * netStreamSource.cpp
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#include "netStreamSource.h"
#include "netAddress.h"
#include "../exceptions/netExceptions.h"
#include "../containers/autoArray.tcc"
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>
#include "linuxFdSelect.h"

namespace LightSpeed {

static int getPortNumber(struct addrinfo *ainfo) {
	if (ainfo->ai_addrlen == sizeof(struct sockaddr_in)) {
		struct sockaddr_in *p = reinterpret_cast<struct sockaddr_in *>(ainfo->ai_addr);
		return htons(p->sin_port);
	} else 	if (ainfo->ai_addrlen == sizeof(struct sockaddr_in6)) {
		struct sockaddr_in6 *p = reinterpret_cast<struct sockaddr_in6 *>(ainfo->ai_addr);
		return htons(p->sin6_port);
	} else return 0;

}

static int createListenSocket(PNetworkAddress addr) {

	LinuxNetAddress *a = dynamic_cast<LinuxNetAddress *>(addr.get());
	if (a == 0) throw NetworkInvalidAddressException(THISLOCATION,addr);
	struct addrinfo *ainfo = a->getAddrInfo();
	if (ainfo == 0)
			throw NetworkInvalidAddressException(THISLOCATION,addr);

	int s = socket(ainfo->ai_family, SOCK_STREAM | SOCK_CLOEXEC, ainfo->ai_protocol);
	if (s == -1) throw NetworkPortOpenException(THISLOCATION,errno,getPortNumber(ainfo));

	int reuse1 = 1;
	setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&reuse1,sizeof(reuse1));
	u_long nonblk = 1;
	ioctl(s,FIONBIO,&nonblk);

	int e = bind(s,ainfo->ai_addr,ainfo->ai_addrlen);
	if (e != 0)
		throw NetworkPortOpenException(THISLOCATION,errno,getPortNumber(ainfo));
	e = listen(s,SOMAXCONN);
	if (e != 0)
		throw NetworkIOError(THISLOCATION,errno,"Listen on socket failed");

	return s;
}


bool LinuxNetAccept::hasItems() const
{
	return count > 0;
}



PNetworkAddress LinuxNetAccept::getLocalAddr() const
{
	if (localAddr == nil) {
		char buff[sizeof(struct sockaddr_in6)];
		unsigned int len = sizeof(buff);
		getsockname(acceptSock,(struct sockaddr *)buff,&len);
		localAddr =  INetworkServices::getNetServices().createAddr(buff,len);
	}
	return localAddr;
}


PNetworkStream LinuxNetAccept::getNext()
{
	if (count == 0)
		throwIteratorNoMoreItems(THISLOCATION,typeid(PNetworkStream));
	wait();
	char buff[256];
	struct sockaddr *saddr = (struct sockaddr *) (buff);
	socklen_t socklen = sizeof(buff);
	int s = accept4(acceptSock, saddr, &socklen,SOCK_CLOEXEC);
	if (s == -1)
		throw NetworkIOError(THISLOCATION, errno, "accept has failed");

	LinuxNetStream* stream = new LinuxNetStream(s, streamDefTimeout);
	remoteAddr = INetworkServices::getNetServices().createAddr(saddr,socklen);
	stream->setTimeout(streamDefTimeout);
	if (count != naturalNull)
		count--;

	return stream;
}

LinuxNetAccept::~LinuxNetAccept() {
	close(acceptSock);
}


natural LinuxNetAccept::doWait(natural , natural timeout) const {

	return safeSelect(acceptSock,waitForInput,timeout);
}




LinuxNetAccept::LinuxNetAccept(PNetworkAddress addr,
		natural count ,natural timeout, natural streamDefTimeout)
	:acceptSock(createListenSocket(addr) )
	,count(count),streamDefTimeout(streamDefTimeout){
	setTimeout(timeout);

}



PNetworkAddress LinuxNetAccept::getPeerAddr() const
{
	return remoteAddr;
}


PNetworkStream LinuxNetConnect::getNext()
{
	if (count == 0)
		throwIteratorNoMoreItems(THISLOCATION,typeid(PNetworkStream));

	PNetworkStream stream = makeConnect();

	if (count != naturalNull) count--;
	stream->setTimeout(streamDefTimeout);
	return stream;

}


PNetworkAddress LinuxNetConnect::getPeerAddr() const
{
	return remoteAddr;
}



PNetworkAddress LinuxNetConnect::getLocalAddr() const
{
	return localAddr;
}



LinuxNetConnect::LinuxNetConnect(PNetworkAddress addr,
		natural count, natural timeout, natural streamDefTimeout)
	:remoteAddr(addr),count(count),
	 streamDefTimeout(streamDefTimeout)
{
	setTimeout(timeout);
}



bool LinuxNetConnect::hasItems() const
{
	return count > 0;
}


static bool probeSocket(int fd) {
	int e = 0;
	socklen_t elen =sizeof(e);
	if (getsockopt(fd,SOL_SOCKET, SO_ERROR, &e, &elen) < 0) {
		return false;
	}
	if (e != 0) {
		return false;
	}
	return true;
}



natural LinuxNetConnect::doWait(natural , natural timeout) const {

	setupSocket();

	LinuxFdSelect fdselect;
	Timeout tm(timeout);
	for (natural i = 0; i < asyncSocket.length(); i++) {
		fdselect.set(asyncSocket[i],waitForOutput,0,tm);
	}

	readySocket = 0;
	do {
		const LinuxFdSelect::FdInfo &s = fdselect.getNext();
		if (s.waitMask == 0) return 0;

		if (probeSocket(s.fd))
			readySocket = s.fd;
		fdselect.unset(s.fd);
	} while (readySocket == 0 && fdselect.hasItems());

	return readySocket?waitForOutput:waitTimeout;
}


integer LinuxNetConnect::getSocket(int idx) const {
	setupSocket();
	natural pos = idx;
	if (pos >= asyncSocket.length()) return -1;
	else return asyncSocket[pos];
}

LinuxNetConnect::~LinuxNetConnect() {
	for (natural i = 0; i < asyncSocket.length();++i)
		close(asyncSocket[i]);
}


void LinuxNetConnect::setupSocket() const {

	if (!asyncSocket.empty()) return; //already set up
	if (count == 0)
		throwIteratorNoMoreItems(THISLOCATION,typeid(PNetworkStream));
	LinuxNetAddress *a = dynamic_cast<LinuxNetAddress *>(remoteAddr.get());
	if (a == 0) throw NetworkInvalidAddressException(THISLOCATION,remoteAddr);

	struct addrinfo *aif = a->getAddrInfo();
	while (aif && asyncSocket.length() < maxWatchSockets) {

		int s = socket(aif->ai_family,SOCK_STREAM | SOCK_CLOEXEC, aif->ai_protocol);
		if (s == -1) {
			if (asyncSocket.empty())
				throw NetworkIOError(THISLOCATION,errno,"socket creation failed");
			else
				break;
		}

		u_long nonblk =1;
		ioctl(s,FIONBIO, &nonblk);
		int e = connect(s,aif->ai_addr,aif->ai_addrlen);
		if (e == -1) {

			int errnr = errno;
			if (errnr != EWOULDBLOCK && errnr != EINPROGRESS) {
				close(s);
				if (asyncSocket.empty())
					throw NetworkIOError(THISLOCATION,errno,"connect failed");
				else
					break;
			}

		}

		asyncSocket.add(s);
		aif= aif->ai_next;
	}


}


PNetworkStream LinuxNetConnect::makeConnect() {
	if (wait() == 0) {
		for (natural i = 0; i < asyncSocket.length(); i++)
			close(asyncSocket[i]);
		asyncSocket.clear();
		throw NetworkConnectFailedException(THISLOCATION,remoteAddr);
	}

	for (natural i = 0; i < asyncSocket.length(); i++)
		if (readySocket != asyncSocket[i]) close(asyncSocket[i]);

	int s = readySocket;
	asyncSocket.clear();

	int e = 0;
	socklen_t elen =sizeof(e);
	if (getsockopt(s,SOL_SOCKET, SO_ERROR, &e, &elen) < 0) {
		close(s);
		throw NetworkIOError(THISLOCATION,errno,"getsockopt failed");
	}
	if (e != 0) {
		close(s);
		throw NetworkConnectFailedException(THISLOCATION,remoteAddr);

	}

	u_long nonblk =1;
	ioctl(s,FIONBIO, &nonblk);

	return new LinuxNetStream(s,streamDefTimeout);

}

}






























