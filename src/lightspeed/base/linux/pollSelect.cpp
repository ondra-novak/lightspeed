/*
 * epollSelect.cpp
 *
 *  Created on: 14. 9. 2015
 *      Author: ondra
 */

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "../exceptions/systemException.h"
#include "../exceptions/invalidParamException.h"

#include "pollSelect.h"

#include "../containers/autoArray.tcc"
#include "../containers/sort.tcc"
#include "../debug/dbglog.h"
#include "../streams/netio_ifc.h"

namespace LightSpeed {

int pipeCloseOnExec(int *fds);

PollSelect::PollSelect():timeoutHeap(timeoutMap) {


	int pipe[2];
	if (pipeCloseOnExec(pipe) == -1)
		throw ErrNoException(THISLOCATION, errno);

	wakeIn = pipe[1];
	wakeOut = pipe[0];

	POLLFD pfd;
	pfd.events = POLL_IN;
	pfd.fd = wakeOut;
	pfd.revents = 0;
	curpool.add(pfd);


}

PollSelect::~PollSelect() {
	close(wakeIn);
	close(wakeOut);
}

void PollSelect::set(int fd, natural waitFor, const Timeout &tm, void* userData) {
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

	if (waitFor == 0) {
		removeFromPool(fdinfo.poolpos);
	} else {


		POLLFD ev;
		ev.fd = fd;
		ev.events = (((waitFor & INetworkResource::waitForInput) != 0)?POLL_IN:0) |
					(((waitFor & INetworkResource::waitForOutput) != 0)?POLL_OUT:0) |
					(((waitFor & INetworkResource::waitForException) != 0)?POLL_PRI:0) ;
		ev.revents = 0;


		if (fdinfo.poolpos ==naturalNull) {
			fdinfo.poolpos = curpool.length();
			curpool.add(ev);
		} else {
			curpool(fdinfo.poolpos) = ev;
		}
	}

}

void PollSelect::wakeUp(natural reason) throw () {
	this->reason = reason;
	(void)write(wakeIn,&reason,1);
}

int PollSelect::getFd(const FdInfo* finfo) {
	return (int)(finfo - socketMap.data());
}

void PollSelect::removeFromPool(natural poolindex) {
	if (poolindex == naturalNull) return;

	socketMap(curpool[poolindex].fd).poolpos = naturalNull;
	if (curpool.length() - 1 != poolindex) {
		curpool(poolindex) = curpool.tail(1)[0];
		socketMap(curpool[poolindex].fd).poolpos = poolindex;
	}
	curpool.trunc(1);
}

PollSelect::WaitStatus PollSelect::wait(const Timeout& tm, Result& result) {

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
					removeFromPool(owner->poolpos);
					return waitEvent;
				}
				if (timeoutHeap.empty()) break;
				p = timeoutHeap.top().owner->timeout;
			}
			if (p < finTm) finTm = p;
		}

		natural timeout_msecs = finTm.getRemain().msecs();
		int res = poll(curpool.data(),curpool.length(),timeout_msecs);
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
				removeFromPool(finfo->poolpos);
				return waitEvent;
			}
		} else {
			for (natural i = 0; i < curpool.length(); i++) {
				if (curpool[i].revents) {
					int fd = curpool[i].fd;
					int revents = curpool[i].revents;

					if (fd == wakeOut) {
						byte b;
						int r = read(wakeOut,&b,1);
						if (r == -1)
							throw ErrNoException(THISLOCATION, errno);
						result.fd = 0;
						result.userData = 0;
						result.reason = reason;
						return waitWakeUp;
					} else  {
						FdInfo *finfo = socketMap.data()+fd;
						removeOldTm(finfo);
						removeFromPool(i);
						if (finfo->waitFor) {
							result.fd = fd;
							result.flags = (((revents & POLL_IN)!=0)?INetworkResource::waitForInput:0) |
									(((revents & POLL_OUT)!=0)?INetworkResource::waitForOutput:0) |
									(((revents & (POLL_PRI|POLL_ERR))!=0)?INetworkResource::waitForException:0);
							result.userData = finfo->userData;
							finfo->waitFor = 0;
							return waitEvent;
						}
					}
				}
			}
		}
	} while (true);



}


bool PollSelect::TimeoutCmp::operator ()(const TmInfo& a, const TmInfo& b) const {
	return a.owner->timeout > b.owner->timeout;
}

void PollSelect::addNewTm(FdInfo* owner) {
	if (owner->timeout.isInfinite()) {
		owner->tmRef = 0;
		return;
	} else {
		timeoutMap.add(TmInfo(owner));
		timeoutHeap.push();
	}
}

void PollSelect::unset(int fd) {
	set(fd,0,nil,0);
}

void* PollSelect::getUserData(int fd) const {
	natural fdindex(fd);
	if (fdindex >= socketMap.length()) return 0;
	else return socketMap[fdindex].userData;
}

void PollSelect::removeOldTm(FdInfo* owner) {
	if (owner->tmRef) {
		natural pos = owner->tmRef - timeoutMap.data();
		owner->tmRef = 0;
		if (pos >= timeoutMap.length()) return;
		timeoutHeap.pop(pos);
		timeoutMap.trunc(1);
	}
}

PollSelect::TmInfo::TmInfo(FdInfo* owner):owner (owner) {
	owner->tmRef = this;

}

PollSelect::TmInfo::TmInfo(const TmInfo& origin):owner(origin.owner) {
	owner->tmRef = this;
}

PollSelect::TmInfo& PollSelect::TmInfo::operator =(const TmInfo& origin) {
	this->owner = origin.owner;
	owner->tmRef = this;
	return *this;
}

void PollSelect::clearHeap() {
	timeoutMap.clear();
	timeoutHeap.clear();

}

void PollSelect::cancelAllVt(const ICancelAllCb& cb) {
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

