/*
 * netStreamSource.cpp
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#include <WinSock2.h>
#include "netStreamSource.h"
#include "netAddress.h"
#include "../exceptions/netExceptions.h"
#include "../containers/autoArray.tcc"
#include <string.h>
#include "../../mt/timeout.h"
#include "../debug/dbglog.h"
#include "winsocket.tcc"

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

static SOCKET createListenSocket(PNetworkAddress addr) {

	WindowsNetAddress *a = dynamic_cast<WindowsNetAddress *>(addr.get());
	if (a == 0) throw NetworkInvalidAddressException(THISLOCATION,addr);
	struct addrinfo *ainfo = a->getAddrInfo();
	if (ainfo == 0)
			throw NetworkInvalidAddressException(THISLOCATION,addr);

	
	SOCKET s = socket(ainfo->ai_family, SOCK_STREAM, ainfo->ai_protocol);
	if (s == -1) throw NetworkPortOpenException(THISLOCATION,WSAGetLastError(),getPortNumber(ainfo));

	//we don't want to inherit sockets!
	SetHandleInformation((HANDLE)s,HANDLE_FLAG_INHERIT,0);


	if (a->isReuseAddrEnabled()) {
		int reuse1 = 1;
		setsockopt(s,SOL_SOCKET,SO_REUSEADDR,(char *)&reuse1,sizeof(reuse1));
	}

	u_long nonblk = 1;
	ioctlsocket(s,FIONBIO,&nonblk);


	if (ainfo->ai_addr->sa_family == PF_INET6) {
		int on = (ainfo->ai_flags & AI_V4MAPPED)?0:1;
		setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY,(char *)&on, sizeof(on));
	}

	int e = bind(s,ainfo->ai_addr,(int)ainfo->ai_addrlen);
	if (e != 0) {
		throw NetworkPortOpenException(THISLOCATION,WSAGetLastError(),getPortNumber(ainfo));
		closesocket(s);
	}
	e = listen(s,SOMAXCONN);
	if (e != 0) {
		throw NetworkIOError(THISLOCATION,WSAGetLastError(),"Listen on socket failed");
		closesocket(s);
	}

	return s;
}


bool WindowsNetAccept::hasItems() const
{
	return count > 0;
}



PNetworkAddress WindowsNetAccept::getLocalAddr() const
{
	if (localAddr == nil) {
		char buff[sizeof(SOCKADDR_IN6)];
		int len = sizeof(buff);
		getsockname(acceptSock,(SOCKADDR *)buff,&len);
		localAddr =  INetworkServices::getNetServices().createAddr(buff,len);
	}
	return localAddr;
}


PNetworkStream WindowsNetAccept::getNext()
{
	if (count == 0)
		throwIteratorNoMoreItems(THISLOCATION,typeid(PNetworkStream));
	wait();
	char buff[256];
	struct sockaddr *saddr = (struct sockaddr *)buff;
	socklen_t socklen = sizeof(buff);

	SOCKET s = accept(acceptSock, saddr, &socklen);
	if (s == INVALID_SOCKET)
		throw NetworkIOError(THISLOCATION, WSAGetLastError(), "accept has failed");

	WindowsNetStream *stream = new WindowsNetStream(s,streamDefTimeout);
	stream->setTimeout(streamDefTimeout);

	remoteAddr = INetworkServices::getNetServices().createAddr(&saddr,socklen);

	if (count != naturalNull) count--;
	return stream;

}



natural WindowsNetAccept::doWait(natural waitFor, natural timeout) const {

	fd_set rdset;
	FD_ZERO(&rdset);
	FD_SET(acceptSock,&rdset);
	struct timeval tm = millisecToTimeval(timeout);
	int e = select(int(acceptSock+1),&rdset,0,0,timeout == naturalNull?0:&tm);
	if (e > 0) return waitForInput;
	if (e == 0) return 0;
	throw NetworkIOError(THISLOCATION,WSAGetLastError(),"select");


}


WindowsNetAccept::WindowsNetAccept(PNetworkAddress addr,
		natural count ,natural timeout, natural streamDefTimeout,
		INetworkServices *svc)
	:acceptSock(createListenSocket(addr) )
	,count(count),streamDefTimeout(streamDefTimeout)
	,svc(svc){
		setTimeout(timeout);

}



PNetworkAddress WindowsNetAccept::getPeerAddr() const
{
	return remoteAddr;
}

WindowsNetAccept::~WindowsNetAccept()
{
	u_long nonblk = 0;
	ioctlsocket(acceptSock,FIONBIO,&nonblk);
	closesocket(acceptSock);
}


PNetworkStream WindowsNetConnect::getNext()
{
	if (count == 0)
		throwIteratorNoMoreItems(THISLOCATION,typeid(PNetworkStream));

	PNetworkStream stream = makeConnect();

	if (count != naturalNull) count--;
	stream->setTimeout(streamDefTimeout);
	return stream;

}


PNetworkAddress WindowsNetConnect::getPeerAddr() const
{
	return remoteAddr;
}



PNetworkAddress WindowsNetConnect::getLocalAddr() const
{
	return localAddr;
}



WindowsNetConnect::WindowsNetConnect(PNetworkAddress addr,
		natural count, natural timeout, natural streamDefTimeout,
		INetworkServices *svc)
	:remoteAddr(addr),count(count),
	 streamDefTimeout(streamDefTimeout)
	 ,svc(svc)
{
	setTimeout(timeout);
}



bool WindowsNetConnect::hasItems() const
{
	return count > 0;
}

natural WindowsNetConnect::doWait(natural waitFor, natural timeout) const {

	setupSocket();
	Timeout tmwait(timeout);

	FD_SET testSet;
	FD_SET wrset;
	FD_SET exset;

	FD_ZERO(&wrset);
	FD_ZERO(&exset);
	FD_ZERO(&testSet);
	

	for (natural i = 0; i < asyncSocket.length(); i++) {
		FD_SET(asyncSocket[i],&testSet);
	}
	
	do {
		
		wrset = testSet;
		exset = testSet;
		DWORD remain = DWORD(tmwait.getRemain().msecs());
		struct timeval tm = millisecToTimeval(remain);
		int e = select(0,0,&wrset,&exset,timeout == naturalNull?0:&tm);
		if (e < 0)
			throw NetworkIOError(THISLOCATION,WSAGetLastError(),"select failed");
		else 
			if (e == 0) return 0;	

		if (wrset.fd_count > 0) {
			readySocket = wrset.fd_array[0];
			break;
		}
		for (unsigned int i = 0; i < exset.fd_count; i++) {
			FD_CLR(exset.fd_array[i],&testSet);
		}
		
		if (testSet.fd_count == 0) {
			readySocket = -1;
			break;
		}
	} while (true);

	return waitForOutput;
}


integer WindowsNetConnect::getSocket(int idx) const {
	setupSocket();
	natural pos = idx;
	if (pos >= asyncSocket.length()) return -1;
	else return asyncSocket[pos];
}

WindowsNetConnect::~WindowsNetConnect() {
	for (natural i = 0; i < asyncSocket.length();++i)
		closesocket(asyncSocket[i]);
}


void WindowsNetConnect::setupSocket() const {

	if (!asyncSocket.empty()) return; //already set up
	if (count == 0)
		throwIteratorNoMoreItems(THISLOCATION,typeid(PNetworkStream));
	WindowsNetAddress *a = dynamic_cast<WindowsNetAddress *>(remoteAddr.get());
	if (a == 0) throw NetworkInvalidAddressException(THISLOCATION,remoteAddr);

	struct addrinfo *aif = a->getAddrInfo();
	while (aif && asyncSocket.length() < maxWatchSockets) {

		SOCKET s = socket(aif->ai_family,SOCK_STREAM, aif->ai_protocol);
		if (s == -1) {
			if (asyncSocket.empty())
				throw NetworkIOError(THISLOCATION,WSAGetLastError(),"socket creation failed");
			else
				break;
		}

		//we don't want to inherit sockets!
		SetHandleInformation((HANDLE)s,HANDLE_FLAG_INHERIT,0);


		int on = 0;
		setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY,(char *)&on, sizeof(on));


		u_long nonblk =1;
		ioctlsocket(s,FIONBIO, &nonblk);
		int e = connect(s,aif->ai_addr,int(aif->ai_addrlen));
		if (e == -1) {

			int errnr = WSAGetLastError();
			if (errnr != WSAEWOULDBLOCK && errnr != WSAEINPROGRESS) {
				closesocket(s);
				if (asyncSocket.empty())
					throw NetworkIOError(THISLOCATION,WSAGetLastError(),"connect failed");
				else
					break;
			}

		}

		asyncSocket.add(s);
		aif= aif->ai_next;
	}


}


PNetworkStream WindowsNetConnect::makeConnect() {
	if (wait() == 0) {
		for (natural i = 0; i < asyncSocket.length(); i++)
			closesocket(asyncSocket[i]);
		asyncSocket.clear();
		throw NetworkConnectFailedException(THISLOCATION,remoteAddr);
	}

	for (natural i = 0; i < asyncSocket.length(); i++)
		if (readySocket != asyncSocket[i]) closesocket(asyncSocket[i]);
	
	if (readySocket == -1)
		throw NetworkIOError(THISLOCATION,WSAGetLastError(),"unable connect");

	SOCKET s = readySocket;
	asyncSocket.clear();

	u_long nonblk =1;
	ioctlsocket(s,FIONBIO, &nonblk);

	return new WindowsNetStream(s,streamDefTimeout);

}

}






























