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

namespace LightSpeed {

int pipeCloseOnExec(int *fds);

WinSelect::WinSelect():timeoutHeap(timeoutMap),rdpos(0),wrpos(0),epos(0) {


	wakefd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (wakefd == INVALID_SOCKET) throw ErrNoException(THISLOCATION, WSAGetLastError());

	SOCKADDR_IN sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_port = 0;

	if (bind(wakefd, (SOCKADDR *)&sin, sizeof(sin)) == SOCKET_ERROR) {
		DWORD err = WSAGetLastError();
		closesocket(wakefd);
		throw ErrNoException(THISLOCATION, err);
	}

}

WinSelect::~WinSelect() {
	closesocket(wakefd);
}

void WinSelect::set(int fd, natural waitFor, Timeout tm, void* userData) {
	if (fd < 0) throw InvalidParamException(THISLOCATION,1,"Invalid descriptor (negative value)");
	natural fdindex = (natural)fd;
	if (fdindex >  1024 && fdindex > socketMap.length() * 4)
				throw InvalidParamException(THISLOCATION,1,"Invalid descriptor (too high value)");
	if (fdindex >= socketMap.length()) {
		const FdInfo *oldMap = socketMap.data();
		socketMap.resize(fdindex+1);
		FdInfo *newMap = socketMap.data();
		if (oldMap != newMap) {
			//because items was moved to new address, we have to recalculate addresses of timeout map
			for (natural i = 0; i < timeoutMap.length(); i++) {
				natural ofs = timeoutMap[i].owner - oldMap;
				timeoutMap(i).owner = newMap + ofs;
			}
		}
	}




	FdInfo &fdinfo = socketMap(fdindex);
	removeOldTm(&fdinfo);

	fdinfo.timeout = tm;
	fdinfo.waitFor = waitFor;
	fdinfo.userData = userData;

	addNewTm(&fdinfo);


}

void WinSelect::wakeUp(natural reason) throw () {

	SOCKADDR_IN sin;	
	int len = sizeof(sin);
	getsockname(wakefd, reinterpret_cast<SOCKADDR *>(&sin), &len);
	sendto(wakefd, reinterpret_cast<const char *>(&len), 1, 0, reinterpret_cast<SOCKADDR *>(&sin), len);
}

int WinSelect::getFd(const FdInfo* finfo) {
	return (int)(finfo - socketMap.data());
}


WinSelect::WaitStatus WinSelect::wait(const Timeout& tm, Result& result) {

	if (readfds.getAllocated() == 0) {
		readfds.reserve(1 + socketMap.length());
		exceptfds.reserve(1 + socketMap.length());
		writefds.reserve(socketMap.length());
	}

	WaitStatus x = walkResult(result);
	if (x != waitNone) return x;

	do {
		Timeout finTm = tm;
		if (!timeoutHeap.empty()) {
			Timeout p = timeoutHeap.top().owner->timeout;
			SysTime now = SysTime::now();
			while (p.expired(now)) {
				FdInfo *owner = timeoutHeap.top().owner;
				removeOldTm(owner);
				if (owner->waitFor != 0) {
					result.fd = getFd(owner);
					result.flags = 0;
					result.userData = owner->userData;
					owner->waitFor = 0;
					return waitEvent;
				}
				if (timeoutHeap.empty()) break;
				p = timeoutHeap.top().owner->timeout;
			}
			if (p < finTm) finTm = p;
		}



		readfds.reset();
		writefds.reset();
		exceptfds.reset();
		wrpos = rdpos = epos = 0;
		readfds.add(wakefd);
		for (natural i = 0, cnt = socketMap.length(); i < cnt; i++) {
			const FdInfo &fdinfo = socketMap[i];
			if (fdinfo.waitFor & INetworkResource::waitForInput) {
				readfds.add((SOCKET)i);
				exceptfds.add((SOCKET)i);
			} 
			if (fdinfo.waitFor & INetworkResource::waitForOutput) {
				writefds.add((SOCKET)i);
			}
		}
				
		struct timeval wtm = finTm.getRemain().getTimeVal();

		int res = select(socketMap.length(), readfds, writefds, exceptfds, finTm.isInfinite() ? 0 : &wtm);
		if (res == -1) {
			if (errno != EINTR) throw ErrNoException(THISLOCATION,WSAGetLastError());
		}
		else if (res == 0) {
			if (tm.expired()) {
				return waitTimeout;
			} else {
				FdInfo *finfo = timeoutHeap.top().owner;
				removeOldTm(finfo);
				result.fd = getFd(finfo);
				result.flags = 0;
				result.userData = finfo->userData;
				finfo->waitFor = 0;
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

bool WinSelect::TimeoutCmp::operator ()(const TmInfo& a, const TmInfo& b) const {
	return a.owner->timeout > b.owner->timeout;
}

void WinSelect::addNewTm(FdInfo* owner) {
	if (owner->timeout.isInfinite()) {
		owner->tmRef = 0;
		return;
	} else {
		timeoutMap.add(TmInfo(owner));
		timeoutHeap.push();
	}
}

void WinSelect::unset(int fd) {
	set(fd,0,nil,0);
}

void* WinSelect::getUserData(int fd) const {
	natural fdindex(fd);
	if (fdindex >= socketMap.length()) return 0;
	else return socketMap[fdindex].userData;
}

void WinSelect::removeOldTm(FdInfo* owner) {
	if (owner->tmRef) {
		natural pos = owner->tmRef - timeoutMap.data();
		owner->tmRef = 0;
		if (pos >= timeoutMap.length()) return;
		timeoutHeap.pop(pos);
		timeoutMap.trunc(1);
	}
}

WinSelect::TmInfo::TmInfo(FdInfo* owner):owner (owner) {
	owner->tmRef = this;

}

WinSelect::TmInfo::TmInfo(const TmInfo& origin):owner(origin.owner) {
	owner->tmRef = this;
}

WinSelect::TmInfo& WinSelect::TmInfo::operator =(const TmInfo& origin) {
	this->owner = origin.owner;
	owner->tmRef = this;
	return *this;
}

void WinSelect::clearHeap() {
	timeoutMap.clear();
	timeoutHeap.clear();

}

LightSpeed::WinSelect::WaitStatus WinSelect::procesResult(const FdSetEx &readfds, natural rdpos, natural type, Result &result)
{
	int fd = readfds->fd_array[rdpos];
	int revents = type;

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
		FdInfo *finfo = socketMap.data() + fd;
		removeOldTm(finfo);
		if (finfo->waitFor) {
			result.fd = fd;
			result.flags = revents;
			result.userData = finfo->userData;
			finfo->waitFor = 0;
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

