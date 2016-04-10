/*
 * linuxFdSelect.cpp
 *
 *  Created on: 13.6.2012
 *      Author: ondra
 */

#include "linuxFdSelect.h"
#include <poll.h>
#include "../streams/netio_ifc.h"
#include <unistd.h>
#include <fcntl.h>
#include "../containers/autoArray.tcc"
#include "../../mt/atomic.h"
#include "../exceptions/netExceptions.h"
#include "../containers/map.tcc"


namespace LightSpeed {


static int waitMaskToEvents(natural waitMask) {
	 int curEv = 0;
	 if (waitMask & INetworkResource::waitForInput) curEv |= POLLIN;
	 if (waitMask & INetworkResource::waitForOutput) curEv |= POLLOUT;
	 if (waitMask & INetworkResource::waitForException) curEv |= POLLPRI;
	 return curEv;
}

static natural eventsToWaitMask(int events) {
	 natural mask = 0;
	 if (events & POLLIN) mask |= INetworkResource::waitForInput;
	 if (events & POLLOUT) mask |= INetworkResource::waitForOutput;
	 if (events & POLLPRI) mask |= INetworkResource::waitForException;
	 if (events & POLLHUP) mask |= INetworkResource::waitForInput;;
	 if (events & POLLNVAL) mask |= INetworkResource::waitForException|INetworkResource::waitForInput|INetworkResource::waitForOutput;
	 return mask;
}

natural safeSelect(integer socket, natural waitFor, natural timeout) {
	struct pollfd fdinfo;
	fdinfo.fd = socket;
	fdinfo.events = waitMaskToEvents(waitFor);
	fdinfo.revents = 0;
	int tm = timeout==naturalNull?-1:(int)timeout;
	int r=poll(&fdinfo,1,tm);
	if (r == 1) {
		return eventsToWaitMask(fdinfo.revents);
	} else if (r == 0) {
		return 0;
	} else {
		throw NetworkIOError(THISLOCATION,errno,"safeSelect (poll)");
	}
}


LinuxFdSelect::LinuxFdSelect():curPos(naturalNull),cancelIn(-1),cancelOut(-1),pendingWakeup(-1) {
}

LinuxFdSelect::~LinuxFdSelect() {
	enableWakeUp(false);
}
void LinuxFdSelect::set(int fd, natural waitMask, void* data, Timeout tmout) {
	set(FdInfo(fd,waitMask,data,tmout));

}

void* LinuxFdSelect::unset(int fd) {
	natural *pos = fdIndex.find(fd);
	if (pos) {
		void *data = fdExtra[*pos].data;
		removeFd(*pos);
		return data;
	} else {
		return 0;
	}
}

void LinuxFdSelect::set(const FdInfo& fdinfo) {
	natural *pos = fdIndex.find(fdinfo.fd);
	if (pos == 0) {
		POLLFD pfd;
		pfd.events = waitMaskToEvents(fdinfo.waitMask);
		pfd.fd = fdinfo.fd;
		pfd.revents = 0;
		FdExtra fex;
		fex.data = fdinfo.data;
		fex.tmout = fdinfo.tmout;
		natural newPos = fdList.length();
		fdList.add(pfd);
		fdExtra.add(fex);
		fdIndex.insert(fdinfo.fd,newPos);
	} else {
		POLLFD &pfd = fdList(*pos);
		FdExtra &fex = fdExtra(*pos);
		pfd.events = waitMaskToEvents(fdinfo.waitMask);
		pfd.revents = 0;
		fex.data = fdinfo.data;
		fex.tmout = fdinfo.tmout;
	}
}

bool LinuxFdSelect::hasItems() const {
	return !fdList.empty();
}

const LinuxFdSelect::FdInfo& LinuxFdSelect::getNext() {
	const FdInfo& r = peek();
	//FIXED: item is no longer signaled - this prevents some problems while curPos is unexpectly reset to zero.
	fdList(curPos).revents = 0;
	curPos++;
	if (r.fd == cancelIn && r.waitMask != 0) {
		writeRelease(&pendingWakeup,-1);
		byte x;
		int err = read(cancelIn,&x,1);
		if (err < 0) throw ErrNoException(THISLOCATION,errno);
	}
	return r;
}

const LinuxFdSelect::FdInfo& LinuxFdSelect::peek() const {


	if (readAcquire(&pendingWakeup) != -1) {
		retVal.data = 0;
		retVal.fd = cancelIn;
		retVal.waitMask = 1;
		retVal.tmout = 0;
		return retVal;
	}

	natural pos = findSignaled();
	if (pos == naturalNull) throwIteratorNoMoreItems(THISLOCATION,typeid(FdInfo));
	retVal.data = fdExtra[pos].data;
	retVal.fd = fdList[pos].fd;
	retVal.waitMask = eventsToWaitMask(fdList[pos].revents);
	retVal.tmout = fdExtra[pos].tmout;
	return retVal;
}

void LinuxFdSelect::dropAll() {
	curPos = 0;
	for (natural i = 0; i < fdList.length(); i++) {
		fdList(i).events = 0;
	}
}

void LinuxFdSelect::removeFd(natural pos) {
	fdIndex.erase(fdList[pos].fd);
	natural lastPos = fdList.length() - 1;
	if (pos  == lastPos) {
		fdList.erase(pos);
		fdExtra.erase(pos);
	} else {
		fdList.set(pos,fdList[lastPos]);
		fdExtra.set(pos,fdExtra[lastPos]);
		fdIndex(fdList[pos].fd) = pos;
		fdList.erase(lastPos);
		fdExtra.erase(lastPos);
	}
	if (curPos > pos) curPos = pos;
}

natural LinuxFdSelect::findSignaled(bool noBlock) const {
	if (fdList.empty()) return naturalNull;
	do {
		while (curPos < fdList.length()) {
			if (fdList[curPos].revents || fdList[curPos].events == 0) {
				return curPos;
			}
			curPos++;
		}
		if (noBlock) return naturalNull;
		doWaitPoll(nil);
	} while (true);
}

bool LinuxFdSelect::isSet(int fd) {
	return fdIndex.find(fd) != 0;
}

void* LinuxFdSelect::getData(int fd) {
	natural *pos = fdIndex.find(fd);
	if (pos) return fdExtra[*pos].data;
	else return 0;
}

void* LinuxFdSelect::getData(int fd, bool& exists) {
	natural *pos = fdIndex.find(fd);
	if (pos) {
		exists = true;
		return fdExtra[*pos].data;
	} else {
		exists = false;
		return 0;
	}
}


int pipeCloseOnExec(int *fds);

void LinuxFdSelect::enableWakeUp(bool enable) {
	if (enable) {
		if (cancelIn == -1 || cancelOut == -1) {
			int fds[2];
			if (pipeCloseOnExec(fds)) {
				throw ErrNoException(THISLOCATION, errno);
			}
			cancelIn = fds[0];
			cancelOut = fds[1];
			pendingWakeup=-1;
		}
		set(cancelIn,waitForInput,0,nil);
	} else {
		if (cancelIn != -1 && cancelOut != -1) {
			unset(cancelIn);
			close(cancelIn);
			close(cancelOut);
		}
	}
}

void LinuxFdSelect::wakeUp(natural reason) throw() {
	this->reason = reason;
	if (lockInc(pendingWakeup) == 0) {
		if (write(cancelOut,&reason,1) < 1) {
			(void)reason;
		}
	}
}

bool LinuxFdSelect::isWakeUpRequest(const FdInfo& fdInfo, natural &reason) const {
	if (fdInfo.fd == cancelIn) {
		reason = this->reason;
		return true;
	} else {
		return false;
	}
}

bool LinuxFdSelect::waitForEvent(const Timeout& timeout) const {
	if (findSignaled(true) != naturalNull) return false;
	return doWaitPoll(timeout);
}

bool LinuxFdSelect::doWaitPoll(const Timeout& tm) const {
	natural minTm = naturalNull;
	SysTime curTm = SysTime::now();
	AutoArray<FdExtra>::Iterator iter = fdExtra.getFwIter();
	while (iter.hasItems()) {
		const Timeout& t = iter.getNext().tmout;
		if (!t.isInfinite()) {
			natural c = t.getRemain(curTm).msecs();
			if (c < minTm)
				minTm = c;
		}
	}

	bool retMask = false;
	if (!tm.isInfinite()) {
		natural reqTm = tm.getRemain(curTm).msecs();
		if (reqTm < minTm) {
			minTm = reqTm;
			retMask = true;
		}
	}

	int finalTm = minTm == naturalNull ? -1 : (int) (minTm);
	int res = poll(fdList.data(),fdList.length(),finalTm);
	if (res == -1) {
		int e = errno;
		if (e == EINTR) {
			return doWaitPoll(tm);
		} else {
			throw ErrNoException(THISLOCATION,e);
		}
	} else {
		SysTime curTm = SysTime::now();
		for (natural i = 0, cnt = fdExtra.length(); i < cnt;  ++i) {
			const Timeout &t = fdExtra[i].tmout;
			if (t.expired(curTm) || (fdList[i].revents & POLLERR) != 0) fdList(i).events = 0;
		}
	}
	curPos = 0;
	return (res == 0) && retMask;
}

}
