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
	:sres(socket,waitForInput),foundEof(false),outputClosed(false) {
	sres.setTimeout(timeout);
}

LinuxNetStream::~LinuxNetStream() {
	shutdown(sres.sock,SHUT_RDWR);
}


LinuxSocketResource::~LinuxSocketResource() {
	if (!noclose) {
		if (handler != nil) handler->close();
		close(sock);
	}
}

Pointer<LinuxSocketResource::IWaitHandler> LinuxSocketResource::getWaitHandler() {
	return handler;
}

natural LinuxSocketResource::getTimeout() const
{
	return timeout;
}



void LinuxSocketResource::setTimeout(natural time_in_ms)
{
	timeout = time_in_ms;
}



natural LinuxSocketResource::userWait(natural waitFor, natural timeout) const
{
	if (handler == nil) return wait(waitFor,timeout);
	else return handler->wait(waitFor, timeout);
}



void LinuxSocketResource::setWaitHandler(IWaitHandler *handler)
{
	this->handler = handler;
}







natural LinuxSocketResource::wait(natural waitFor, natural timeout) const
{
	if (waitFor == 0) waitFor = waitForInput;

	return safeSelect(sock,waitFor,timeout);
}


natural LinuxNetStream::wait(natural waitFor, natural timeout) const {
	return sres.userWait(waitFor,timeout);
}



 void LinuxNetStream::setWaitHandler(IWaitHandler *handler) {
	return sres.setWaitHandler(handler);
}
 void LinuxNetStream::setTimeout(natural time_in_ms) {
	return sres.setTimeout(time_in_ms);
}
 natural LinuxNetStream::getTimeout() const {
	return sres.getTimeout();
}

natural LinuxNetStream::read( void *buffer, natural size )
{
	if (foundEof) return 0;
	if (!wait(INetworkResource::waitForInput))
		throw NetworkTimeoutException(THISLOCATION,sres.getTimeout(),
					NetworkTimeoutException::reading);
	int res = recv(sres.sock,reinterpret_cast<char *>(buffer),(int)size,0);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,errno,"recv failed");
	if (res == 0) foundEof = true;;
	return (natural)res;

}

natural LinuxNetStream::write( const void *buffer, natural size )
{
	if (foundEof)
		throw NetworkIOError(THISLOCATION,0,"send failed - connection closed");
	if (!wait(INetworkResource::waitForOutput))
		throw NetworkTimeoutException(THISLOCATION,sres.getTimeout(),
			NetworkTimeoutException::writing);
	int res = send(sres.sock,reinterpret_cast<const char *>(buffer),(int)size,0);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,errno,"send failed");
	return (natural)res;

}

natural LinuxNetStream::peek( void *buffer, natural size ) const
{
	if (foundEof) return 0;
	if (buffer == 0) {
		u_long r = 0;
		ioctl(sres.sock,FIONREAD,&r);
		return (natural)r;
	}

	if (!wait(INetworkResource::waitForInput))
		throw NetworkTimeoutException(THISLOCATION,sres.getTimeout(),
		NetworkTimeoutException::reading);
	int res = recv(sres.sock,reinterpret_cast<char *>(buffer),(int)size,MSG_PEEK);
	if (res == -1)
		throw NetworkIOError(THISLOCATION,errno,"recv failed");
	if (res == 0) foundEof = true;
	return (natural)res;
}

bool LinuxNetStream::canWrite() const
{
	return !outputClosed;
}

bool LinuxNetStream::canRead() const {
	if (foundEof) return false;
	if (wait(INetworkResource::waitForInput,0) == 0)
		return true;
	return peek(0,0) > 0;
}

integer LinuxNetStream::getSocket(int idx) const {
	if (idx == 0) return sres.sock;
	else return -1;
}

natural LinuxNetStream::getDefaultWait() const {
	return waitForInput;
}

natural LinuxNetStream::dataReady() const {
	if (foundEof) return 1;
	return peek(0,0) > 0;
}

void LinuxNetStream::closeOutput() {
	flush();
	outputClosed = true;
	shutdown(sres.sock,SHUT_WR);
}


}

