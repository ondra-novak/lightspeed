/*
 * netStream.cpp
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#include <WinSock2.h>
#include "netStream.h"
#include "../exceptions/netExceptions.h"
#include <algorithm>


namespace LightSpeed {



WindowsNetStream::WindowsNetStream(UINT_PTR socket, natural timeout)
:sres(socket,waitForInput),outputClosed(false) {
	sres.setTimeout(timeout);
}

WindowsNetStream::~WindowsNetStream() {
	// TODO Auto-generated destructor stub
}


WindowsSocketResource::~WindowsSocketResource() {
	if (!noclose) {
		if (handler != nil) handler->close();
		closesocket(sock);
	}
}

Pointer<WindowsSocketResource::IWaitHandler> WindowsSocketResource::getWaitHandler() {
	return handler;
}

natural WindowsSocketResource::getTimeout() const
{
	return timeout;
}



void WindowsSocketResource::setTimeout(natural time_in_ms)
{
	timeout = time_in_ms;
}



natural WindowsSocketResource::userWait(natural waitFor, natural timeout) const
{
	if (handler == nil) return wait(waitFor,timeout);
	else return handler->wait(waitFor, timeout);
}



void WindowsSocketResource::setWaitHandler(IWaitHandler *handler)
{
	this->handler = handler;
}







natural WindowsSocketResource::wait(natural waitFor, natural timeout) const
{
	if (waitFor == 0) waitFor = waitForInput;

	FD_SET inputSet,outputSet,exceptSet;
	FD_SET *ptrin,*ptrout,*ptrexc;
	if (waitFor & waitForInput) {
		FD_ZERO(&inputSet);
		FD_SET(sock,&inputSet);
		ptrin = &inputSet;
	} else {
		ptrin = 0;
	}
	if (waitFor & waitForOutput) {
		FD_ZERO(&outputSet);
		FD_SET(sock,&outputSet);
		ptrout = &outputSet;
	} else {
		ptrout = 0;
	}
	if (waitFor & waitForException) {
		FD_ZERO(&exceptSet);
		FD_SET(sock,&exceptSet);
		ptrexc = &exceptSet;
	} else {
		ptrexc = 0;
	}

	struct timeval tm = millisecToTimeval(timeout);
	int res = select(int(sock+1),ptrin,ptrout,ptrexc,timeout==naturalNull?0:&tm);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,WSAGetLastError(),"select failed");
	if (res == 0)
		return waitTimeout;
	natural out = 0;
	if (ptrin && FD_ISSET(sock,ptrin))
		out |= waitForInput;
	if (ptrout && FD_ISSET(sock,ptrout))
		out |= waitForOutput;
	if (ptrexc && FD_ISSET(sock,ptrexc))
		out |= waitForException;
	return out;
}


natural WindowsNetStream::wait(natural waitFor, natural timeout) const {
	return sres.userWait(waitFor,timeout);
}



 void WindowsNetStream::setWaitHandler(IWaitHandler *handler) {
	return sres.setWaitHandler(handler);
}
 void WindowsNetStream::setTimeout(natural time_in_ms) {
	return sres.setTimeout(time_in_ms);
}
 natural WindowsNetStream::getTimeout() const {
	return sres.getTimeout();
}

natural WindowsNetStream::read( void *buffer, natural size )
{
	if (!wait(INetworkResource::waitForInput))
		throw NetworkTimeoutException(THISLOCATION,sres.getTimeout(),
					NetworkTimeoutException::reading);
	int res = recv(sres.sock,reinterpret_cast<char *>(buffer),(int)size,0);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,WSAGetLastError(),"recv failed");
	return (natural)res;

}

natural WindowsNetStream::write( const void *buffer, natural size )
{
	if (!wait(INetworkResource::waitForOutput))
		throw NetworkTimeoutException(THISLOCATION,sres.getTimeout(),
			NetworkTimeoutException::writing);
	int res = send(sres.sock,reinterpret_cast<const char *>(buffer),(int)size,0);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,WSAGetLastError(),"send failed");
	return (natural)res;

}

natural WindowsNetStream::peek( void *buffer, natural size ) const
{
	if (buffer == 0) {
		return getReadyBytes();

	}

	if (!wait(INetworkResource::waitForInput))
		throw NetworkTimeoutException(THISLOCATION,sres.getTimeout(),
		NetworkTimeoutException::reading);
	int res = recv(sres.sock,reinterpret_cast<char *>(buffer),(int)size,MSG_PEEK);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,WSAGetLastError(),"recv failed");
	return (natural)res;
}

bool WindowsNetStream::canWrite() const
{
	return !outputClosed;
}

bool WindowsNetStream::canRead() const {
	//if no data ready, we can read with blocking
	if (wait(INetworkResource::waitForInput,0) == 0)
		return true;
	//if data ready, count must be above 0, otherwise EOF
	return getReadyBytes() > 0;
}

integer WindowsNetStream::getSocket(int idx) const {
	if (idx == 0) return sres.sock;
	else return -1;
}

natural WindowsNetStream::getDefaultWait() const {
	return waitForInput;
}

natural WindowsNetStream::dataReady() const
{
	//return false, if no data ready on socket
	if (wait(INetworkResource::waitForInput,0) == 0)
		return 0;
	else {
		//otherwise return true
		return std::min(getReadyBytes(),natural(1));
	}
}

LightSpeed::natural WindowsNetStream::getReadyBytes() const
{
	u_long res = 0;
	ioctlsocket(sres.sock,FIONREAD,&res);
	return (LightSpeed::natural)res;
}

void WindowsNetStream::closeOutput()
{
	flush();
	outputClosed = true;
	shutdown(sres.sock,SD_SEND);
}

}
