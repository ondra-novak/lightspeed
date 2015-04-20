/*
 * netStream.cpp
 *
 *  Created on: 17.4.2011
 *      Author: ondra
 */

#include "netStream.h"
#include <unistd.h>
#include <sys/select.h>
#include "../exceptions/netExceptions.h"
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <poll.h>
#include "linsocket.tcc"

namespace LightSpeed {

struct timeval millisecToTimeval(natural millisec) {
	struct timeval val;
	if (millisec == naturalNull) {
		val.tv_sec = ~0;
		val.tv_usec = 0;
	} else {
		val.tv_sec = millisec / 1000;
		val.tv_usec = (millisec % 1000) * 1000;
	}
	return val;
}


LinuxNetStream::LinuxNetStream(int socket, natural timeout)
	:LinuxSocketResource<INetworkStream>(socket,waitForInput),foundEof(false),countReady(0),outputClosed(false) {
	setTimeout(timeout);
}

LinuxNetStream::~LinuxNetStream() {
	shutdown(sock,SHUT_RDWR);
}



natural LinuxNetStream::read( void *buffer, natural size )
{
	if (foundEof) return 0;
	if (!wait(INetworkResource::waitForInput))
		throw NetworkTimeoutException(THISLOCATION,getTimeout(),
					NetworkTimeoutException::reading);
	int res = recv(sock,reinterpret_cast<char *>(buffer),(int)size,0);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,errno,"recv failed");
	if (res == 0) foundEof = true;;
	natural rd = (natural)res;
	if (rd > countReady) countReady = 0; else countReady -= rd;
	return rd;

}

natural LinuxNetStream::write( const void *buffer, natural size )
{
	if (foundEof)
		throw NetworkIOError(THISLOCATION,0,"send failed - connection closed");
	if (!wait(INetworkResource::waitForOutput))
		throw NetworkTimeoutException(THISLOCATION,getTimeout(),
			NetworkTimeoutException::writing);
	int res = send(sock,reinterpret_cast<const char *>(buffer),(int)size,0);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,errno,"send failed");
	return (natural)res;

}

natural LinuxNetStream::peek( void *buffer, natural size ) const
{
	if (foundEof) return 0;

	if (!wait(INetworkResource::waitForInput))
		throw NetworkTimeoutException(THISLOCATION,getTimeout(),
		NetworkTimeoutException::reading);
	int res = recv(sock,reinterpret_cast<char *>(buffer),(int)size,MSG_PEEK);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,errno,"recv failed");
	if (res == 0) foundEof = true;
	natural rd = (natural)res;
	if (rd > countReady) countReady = rd;
	return rd;
}

bool LinuxNetStream::canWrite() const
{
	return !outputClosed;
}

bool LinuxNetStream::canRead() const {
	if (foundEof) return false;
	if (countReady > 0) return true;
	if (wait(waitForInput) & waitForInput) {
		u_long r = 0;
		ioctl(sock,FIONREAD,&r);
		countReady = (natural)r;
		return countReady > 0;
	} else {
		throw NetworkTimeoutException(THISLOCATION,getTimeout(),NetworkTimeoutException::reading);
	}

}

integer LinuxNetStream::getSocket(int idx) const {
	if (idx == 0) return sock;
	else return -1;
}

natural LinuxNetStream::getDefaultWait() const {
	return waitForInput;
}

natural LinuxNetStream::dataReady() const {
	if (foundEof) return 1;
	if (countReady > 0) return countReady;
	u_long r = 0;
	ioctl(sock,FIONREAD,&r);
	countReady = (natural)r;
	if (countReady > 0) return countReady;
	if (wait(waitForInput,0) & waitForInput) {
		ioctl(sock,FIONREAD,&r);
		countReady = (natural)(r == 0?1:r);
		return countReady;
	} else {
		return 0;
	}
}

void LinuxNetStream::closeOutput() {
	flush();
	outputClosed = true;
	shutdown(sock,SHUT_WR);
}


}

