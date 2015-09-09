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
#include "winsocket.tcc"


namespace LightSpeed {



WindowsNetStream::WindowsNetStream(UINT_PTR socket, natural timeout)
:WindowsSocketResource<INetworkStream>(socket,waitForInput),foundEof(false),countReady(0)
,outputClosed(false) {
	setTimeout(timeout);
}

WindowsNetStream::~WindowsNetStream() {
	// TODO Auto-generated destructor stub
}


natural WindowsNetStream::read( void *buffer, natural size )
{
	if (foundEof) return 0;
	if (!wait(INetworkResource::waitForInput))
		throw NetworkTimeoutException(THISLOCATION,getTimeout(),
					NetworkTimeoutException::reading);
	int res = recv(sock,reinterpret_cast<char *>(buffer),(int)size,0);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,WSAGetLastError(),"recv failed");
	if (res == 0) foundEof = true;;
	natural rd = (natural)res;
	if (rd > countReady) countReady = 0; else countReady -= rd;
	return rd;

}

natural WindowsNetStream::write( const void *buffer, natural size )
{
	if (foundEof)
		throw NetworkIOError(THISLOCATION,0,"send failed - connection closed");
	if (!wait(INetworkResource::waitForOutput))
		throw NetworkTimeoutException(THISLOCATION,getTimeout(),
			NetworkTimeoutException::writing);
	int res = send(sock,reinterpret_cast<const char *>(buffer),(int)size,0);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,WSAGetLastError(),"send failed");
	return (natural)res;

}

natural WindowsNetStream::peek( void *buffer, natural size ) const
{
	if (foundEof) return 0;

	if (!wait(INetworkResource::waitForInput))
		throw NetworkTimeoutException(THISLOCATION,getTimeout(),
		NetworkTimeoutException::reading);
	int res = recv(sock,reinterpret_cast<char *>(buffer),(int)size,MSG_PEEK);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,WSAGetLastError(),"recv failed");
	if (res == 0) foundEof = true;
	natural rd = (natural)res;
	if (rd > countReady) countReady = rd;
	return rd;
}

bool WindowsNetStream::canWrite() const
{
	return !outputClosed;
}

bool WindowsNetStream::canRead() const {
	if (foundEof) return false;
	if (countReady > 0) return true;
	if (wait(waitForInput) & waitForInput) {
		u_long r = 0;
		ioctlsocket(sock,FIONREAD,&r);
		countReady = (natural)r;
		return countReady > 0;
	} else {
		throw NetworkTimeoutException(THISLOCATION,getTimeout(),NetworkTimeoutException::reading);
	}
}

integer WindowsNetStream::getSocket(int idx) const {
	if (idx == 0) return sock;
	else return -1;
}

natural WindowsNetStream::getDefaultWait() const {
	return waitForInput;
}

natural WindowsNetStream::dataReady() const {
	if (foundEof) return 1;
	if (countReady > 0) return countReady;
	u_long r = 0;
	ioctlsocket(sock,FIONREAD,&r);
	countReady = (natural)r;
	if (countReady > 0) return countReady;
	if (wait(waitForInput,0) & waitForInput) {
		ioctlsocket(sock,FIONREAD,&r);
		countReady = (natural)(r == 0?1:r);
		return countReady;
	} else {
		return 0;
	}
}

natural WindowsNetStream::getReadyBytes() const
{
	u_long res = 0;
	ioctlsocket(sock,FIONREAD,&res);
	return (natural)res;
}

void WindowsNetStream::closeOutput()
{
	flush();
	outputClosed = true;
	shutdown(sock,SD_SEND);
}

}
