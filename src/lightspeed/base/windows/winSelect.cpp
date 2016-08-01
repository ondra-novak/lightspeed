/*
 * eWinSelect.cpp
 *
 *  Created on: 14. 9. 2015
 *      Author: ondra
 */

#include "winpch.h"
#include "winSelect.h"
#include "../exceptions/systemException.h"
#include "../exceptions/invalidParamException.h"
#include "../containers/autoArray.tcc"
#include "../containers/sort.tcc"
#include "../debug/dbglog.h"
#include "../streams/netio_ifc.h"
#include <ws2ipdef.h>
#include "../containers/map.tcc"

namespace LightSpeed {

int pipeCloseOnExec(int *fds);

WinSelect::WinSelect():rdpos(0),wrpos(0),epos(0) {


	wakefd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (wakefd == INVALID_SOCKET) throw ErrNoException(THISLOCATION, WSAGetLastError());

	SOCKADDR_IN sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.S_un.S_un_b.s_b1 = 127;
	sin.sin_addr.S_un.S_un_b.s_b2 = 0;
	sin.sin_addr.S_un.S_un_b.s_b3 = 0;
	sin.sin_addr.S_un.S_un_b.s_b4 = 1;
	sin.sin_port = 0;

	if (bind(wakefd, (SOCKADDR *)&sin, sizeof(sin)) == SOCKET_ERROR) {
		DWORD err = WSAGetLastError();
		closesocket(wakefd);
		throw ErrNoException(THISLOCATION, err);
	}

	int len = sizeof(wakeaddr);
	getsockname(wakefd, reinterpret_cast<SOCKADDR *>(&wakeaddr), &len);

}

WinSelect::~WinSelect() {
	closesocket(wakefd);
}

void WinSelect::set(int fd, natural waitFor, Timeout tm, void* userData) {


	SOCKET s = (SOCKET)fd;

	if (waitFor == 0) {
		bool found;
		SocketMap::Iterator iter = socketMap.seek(fd,&found);
		if (found) {
			const FdInfo &fdinfo = iter.peek().value;
			if (fdinfo.activeIndex != naturalNull) {
				if (fdinfo.activeIndex < activeSockets.length() - 1) {
					std::swap(activeSockets(fdinfo.activeIndex), activeSockets(activeSockets.length() - 1));
					activeSockets[fdinfo.activeIndex]->activeIndex = fdinfo.activeIndex;
				}
				activeSockets.trunc(1);
			}
			socketMap.erase(iter);
		}
	}
	else {

		FdInfo &fdinfo = socketMap(s);
		fdinfo.socket = s;


		fdinfo.timeout = tm;
		fdinfo.waitFor = waitFor;
		fdinfo.userData = userData;


		if (fdinfo.activeIndex == naturalNull) {
			fdinfo.activeIndex = activeSockets.length();
			activeSockets.add(&fdinfo);
		}
	}

}

void WinSelect::wakeUp(natural reason) throw () {

	byte b = 1;
	sendto(wakefd, reinterpret_cast<const char *>(&b), 1, 0, reinterpret_cast<SOCKADDR *>(&wakeaddr), sizeof(wakeaddr));
}

int WinSelect::getFd(const FdInfo* finfo) {
	return finfo->socket;
}


WinSelect::WaitStatus WinSelect::wait(const Timeout& tm, Result& result) {


	readfds.reserve(1 + activeSockets.length());
	exceptfds.reserve(1 + activeSockets.length());
	writefds.reserve(activeSockets.length());

	WaitStatus x = walkResult(result);
	if (x != waitNone) return x;

	do {
		Timeout finTm = tm;
		const FdInfo *expireSoon = 0;


		readfds.reset();
		writefds.reset();
		exceptfds.reset();
		wrpos = rdpos = epos = 0;
		readfds.add(wakefd);
		for (natural i = 0, cnt = activeSockets.length(); i < cnt; i++) {
			const FdInfo *fdinfo = activeSockets[i];
			if (fdinfo->waitFor & INetworkResource::waitForInput) {
				readfds.add((SOCKET)fdinfo->socket);
				exceptfds.add((SOCKET)fdinfo->socket);
			} 
			if (fdinfo->waitFor & INetworkResource::waitForOutput) {
				writefds.add((SOCKET)fdinfo->socket);
			}
			if (fdinfo->timeout < finTm) {
				expireSoon = fdinfo;
				finTm = fdinfo->timeout;
			}
		}
				
		struct timeval wtm = finTm.getRemain().getTimeVal();

		int res = select((int)socketMap.length(), readfds, writefds, exceptfds, finTm.isInfinite() ? 0 : &wtm);
		if (res == -1) {
			if (errno != EINTR) throw ErrNoException(THISLOCATION,WSAGetLastError());
		}
		else if (res == 0) {
			if (tm.expired()) {
				return waitTimeout;
			} else {
				const FdInfo *finfo = expireSoon;
				result.fd = getFd(finfo);
				result.flags = 0;
				result.userData = finfo->userData;
				unset(finfo->socket);
				return waitEvent;
			}
		} else {

			return walkResult(result);
			
		}
	} while (true);



}


LightSpeed::WinSelect::WaitStatus WinSelect::walkResult(Result &result)
{
	if (rdpos < readfds->fd_count) {
		return procesResult(readfds, rdpos++, INetworkResource::waitForInput,result);
	}
	if (wrpos < writefds->fd_count) {
		return procesResult(writefds, wrpos++, INetworkResource::waitForOutput, result);
	}
	if (epos < exceptfds->fd_count) {
		return procesResult(exceptfds, epos++, INetworkResource::waitForInput, result);
	}
	return waitNone;
}



void WinSelect::unset(int fd) {
	set(fd,0,nil,0);
}

void* WinSelect::getUserData(int fd) const {
	const FdInfo *r = socketMap.find(fd);
	if (r) return r->userData;
	else return 0;
}




LightSpeed::WinSelect::WaitStatus WinSelect::procesResult(const FdSetEx &readfds, natural rdpos, natural type, Result &result)
{
	SOCKET fd = readfds->fd_array[rdpos];
	natural revents = type;

	if (fd == wakefd) {
		char b;
		SOCKADDR_IN addr;
		int sinlen = sizeof(addr);
		int r = recvfrom(wakefd, &b, 1, 0,(SOCKADDR *)&addr, &sinlen);
		if (r == -1)
			throw ErrNoException(THISLOCATION, errno);
		result.fd = fd;
		result.userData = 0;
		result.reason = reason;
		return waitWakeUp;
	}
	else  {
		FdInfo *finfo = socketMap.find(fd);
		if (finfo) {
			result.fd = fd;
			result.flags = revents;
			result.userData = finfo->userData;
			unset(fd);
			return waitEvent;
		}
	}
	return waitNone;

}

WinSelect::FdSetEx::FdSetEx() :allocated(0), fdset(0)
{

}

WinSelect::FdSetEx::FdSetEx(const FdSetEx &other)
{
	reserve(other->fd_count);
	for (natural i = 0; i < other->fd_count; i++) {
		add(other->fd_array[i]);
	}
}

WinSelect::FdSetEx & WinSelect::FdSetEx::operator=(const FdSetEx &other)
{
	if (&other != this) {
		reset();
		reserve(other->fd_count);
		for (natural i = 0; i < other->fd_count; i++) {
			add(other->fd_array[i]);
		}
	}
	return *this;
}

WinSelect::FdSetEx::~FdSetEx()
{
	free(fdset);
}

void WinSelect::FdSetEx::reserve(natural count)
{
	natural needsz = offsetof(fd_set, fd_array) + sizeof(SOCKET) * count;

	
	if (fdset == 0) {
		fdset = (fd_set *)malloc(needsz);
		fdset->fd_count = 0;
		allocated = count;
	}
	else if (allocated< count) {
		fdset = (fd_set *)realloc(fdset,needsz);
		allocated = count;
	}
}

void WinSelect::FdSetEx::reset()
{
	fdset->fd_count = 0;
}

void WinSelect::FdSetEx::add(SOCKET s)
{
	fdset->fd_array[fdset->fd_count] = s;
	fdset->fd_count++;
}

} /* namespace LightSpeed */

