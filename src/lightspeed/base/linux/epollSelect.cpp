/*
 * epollSelect.cpp
 *
 *  Created on: 14. 9. 2015
 *      Author: ondra
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "../exceptions/systemException.h"
#include "../exceptions/invalidParamException.h"

#include "epollSelect.h"

#include "../containers/autoArray.tcc"
#include "../containers/sort.tcc"
#include "../debug/dbglog.h"
#include "../streams/netio_ifc.h"

namespace LightSpeed {

int pipeCloseOnExec(int *fds);

EPollSelect::EPollSelect():timeoutHeap(timeoutMap) {

	epollfd = epoll_create1(EPOLL_CLOEXEC);
	if (epollfd == -1)
		throw ErrNoException(THISLOCATION, errno);

	int pipe[2];
	if (pipeCloseOnExec(pipe) == -1)
		throw ErrNoException(THISLOCATION, errno);

	wakeIn = pipe[1];
	wakeOut = pipe[0];

	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = wakeOut;
	if (epoll_ctl(epollfd,EPOLL_CTL_ADD,wakeOut,&ev) == -1)
		throw ErrNoException(THISLOCATION, errno);

}

EPollSelect::~EPollSelect() {
	close(epollfd);
	close(wakeIn);
	close(wakeOut);
}

void EPollSelect::set(int fd, natural waitFor, const Timeout &tm, void* userData) {
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

	struct epoll_event ev;
	static const EPOLL_EVENTS EPOLLNONE = (EPOLL_EVENTS)0;
	ev.data.fd = fd;
	ev.events = (((waitFor & INetworkResource::waitForInput) != 0)?EPOLLIN:EPOLLNONE) |
				(((waitFor & INetworkResource::waitForOutput) != 0)?EPOLLOUT:EPOLLNONE) |
				(((waitFor & INetworkResource::waitForException) != 0)?EPOLLPRI:EPOLLNONE) | EPOLLONESHOT;


	if (epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev) == -1) {
		int err = errno;
		if (err == ENOENT) {
			if (epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev)) {
				err = errno;
				throw ErrNoException(THISLOCATION, errno);
			}
		} else {
			throw ErrNoException(THISLOCATION, errno);
		}
	}
}

void EPollSelect::wakeUp(natural reason) throw () {
	this->reason = reason;
	(void)write(wakeIn,&reason,1);
}

int EPollSelect::getFd(const FdInfo* finfo) {
	return (int)(finfo - socketMap.data());
}

EPollSelect::WaitStatus EPollSelect::wait(const Timeout& tm, Result& result) {

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

		struct epoll_event ev;
		natural timeout_msecs = finTm.getRemain().msecs();
		int res = epoll_wait(epollfd,&ev,1,timeout_msecs);
		if (res == -1) {
			if (errno != EINTR) throw ErrNoException(THISLOCATION,errno);
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
			if (ev.data.fd == wakeOut) {
				byte b;
				int r = read(wakeOut,&b,1);
				if (r == -1)
					throw ErrNoException(THISLOCATION, errno);
				result.fd = 0;
				result.userData = 0;
				result.reason = reason;
				return waitWakeUp;
			} else  {
				FdInfo *finfo = socketMap.data()+ev.data.fd;
				removeOldTm(finfo);

				if (finfo->waitFor) {
					result.fd = ev.data.fd;
					result.flags = (((ev.events & EPOLLIN)!=0)?INetworkResource::waitForInput:0) |
							(((ev.events & EPOLLOUT)!=0)?INetworkResource::waitForOutput:0) |
							(((ev.events & (EPOLLPRI|EPOLLERR))!=0)?INetworkResource::waitForException:0);
					result.userData = finfo->userData;
					finfo->waitFor = 0;
					return waitEvent;
				}
			}
		}
	} while (true);



}


bool EPollSelect::TimeoutCmp::operator ()(const TmInfo& a, const TmInfo& b) const {
	return a.owner->timeout > b.owner->timeout;
}

void EPollSelect::addNewTm(FdInfo* owner) {
	if (owner->timeout.isInfinite()) {
		owner->tmRef = 0;
		return;
	} else {
		timeoutMap.add(TmInfo(owner));
		timeoutHeap.push();
	}
}

void EPollSelect::unset(int fd) {
	set(fd,0,nil,0);
}

void* EPollSelect::getUserData(int fd) const {
	natural fdindex(fd);
	if (fdindex >= socketMap.length()) return 0;
	else return socketMap[fdindex].userData;
}

void EPollSelect::removeOldTm(FdInfo* owner) {
	if (owner->tmRef) {
		natural pos = owner->tmRef - timeoutMap.data();
		owner->tmRef = 0;
		if (pos >= timeoutMap.length()) return;
		timeoutHeap.pop(pos);
		timeoutMap.trunc(1);
	}
}

EPollSelect::TmInfo::TmInfo(FdInfo* owner):owner (owner) {
	owner->tmRef = this;

}

EPollSelect::TmInfo::TmInfo(const TmInfo& origin):owner(origin.owner) {
	owner->tmRef = this;
}

EPollSelect::TmInfo& EPollSelect::TmInfo::operator =(const TmInfo& origin) {
	this->owner = origin.owner;
	owner->tmRef = this;
	return *this;
}

void EPollSelect::cancelAllVt(const ICancelAllCb& cb) {
	for (natural i = 0; i < socketMap.length(); i++) {
		if (socketMap[i].waitFor != 0) {
			cb(i, socketMap[i].userData);
			socketMap(i).waitFor = 0;
			socketMap(i).tmRef = 0;
		}
	}
	timeoutMap.clear();
	timeoutHeap.clear();

}


} /* namespace LightSpeed */

