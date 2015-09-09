/*
 * ePollListener.cpp
 *
 *  Created on: 9. 9. 2015
 *      Author: ondra
 */

#include <unistd.h>
#include <sys/epoll.h>
#include "ePollListener.h"




namespace LightSpeed {


EPollListener::EPollListener():socketTimeouts(socketHeapCont) {
	wakePipeFds[0] = -1;
}

EPollListener::~EPollListener() {
	lock.lock();
	if (wakePipeFds[0] != -1) {
		worker.finish();
		lock.unlock();
		worker.join();
		lock.lock();
		close(wakePipeFds[0]);
		close(wakePipeFds[1]);
	}
}


void EPollListener::set(const Request& request) {

	this->execute(Action::create(this,&EPollListener::onRequest,request));

}

int pipeCloseOnExec(int *fds);


void EPollListener::notify() {
	if (wakePipeFds[0] == -1) {
		Synchronized<FastLock> _(lock);
		if (wakePipeFds[0] == -1) {
			pipeCloseOnExec(wakePipeFds);
			worker.start(ThreadFunction::create(this,&EPollListener::workerProc));
		}
	}
	byte b = 1;
	(void)write(wakePipeFds[1], &b,1);
}


void EPollListener::regFd(int epoll, natural waitFor, int fd, FdDesc *desc) {

	struct epoll_event ev;
	int events= 0;
	if (waitFor & INetworkResource::waitForInput) events |= EPOLLIN;
	if (waitFor & INetworkResource::waitForOutput) events |= EPOLLOUT;

	ev.events = events;
	ev.data.ptr = desc;

	int r = epoll_ctl(epoll,EPOLL_CTL_ADD, fd, &ev);
	if (r == -1) throw ErrNoException(THISLOCATION, errno);
}

void EPollListener::unregFd(int epoll, int fd) {
	struct epoll_event ev;

	int r = epoll_ctl(epoll,EPOLL_CTL_DEL, fd, &ev);
	if (r == -1) throw ErrNoException(THISLOCATION, errno);
}

void EPollListener::workerProc() {

	int epollfd = epoll_create1(EPOLL_CLOEXEC);
	if (epollfd == -1) throw ErrNoException(THISLOCATION, errno);

	this->epoll_fd = epollfd;
	struct epoll_event events[100];

	regFd(epollfd, INetworkResource::waitForInput, wakePipeFds[0],0);

	while (!Thread::canFinish()) {

		int tm = -1;
		if (!socketTimeouts.empty()) {
			RefCntPtr<FdDesc> n = socketTimeouts.top();
			tm = (int)(n->timeout.getRemain().msecs());
		}

		int cnt = epoll_wait(epollfd,events,100,tm);
		if (cnt == -1)
			throw ErrNoException(THISLOCATION, errno);
		if (cnt == 0) {
			SysTime now = SysTime::now();
			do {
				RefCntPtr<FdDesc> n = socketTimeouts.top();
				if (!n->timeout.expired(now)) {
					break;
				}

				socketTimeouts.pop();
				socketHeapCont.resize(socketTimeouts.getSize());

				eraseReg(n);
				n->ntf->wakeUp(0);

			} while (true);
		} else {
			for (int i = 0; i < cnt; i++) {
				RefCntPtr<FdDesc> d = reinterpret_cast<FdDesc *>(events[i].data.ptr);
				eraseReg(d);
				eraseTimeout(d);
				natural waitFor = 0;
				if (events[i].events & EPOLLIN) waitFor|=INetworkResource::waitForInput;
				if (events[i].events & EPOLLOUT) waitFor|=INetworkResource::waitForOutput;
				if (events[i].events & EPOLLERR) waitFor|=INetworkResource::waitForException;
				d->ntf->wakeUp(waitFor);
			}
		}
	}
}

void EPollListener::onRequest(const Request& request) {

	INetworkSocket &sck = request.rsrc->getIfc<INetworkSocket>();

	RefCntPtr<FdDesc> desc = new(alloc) FdDesc(request.rsrc,request.timeout_ms, request.notify);

	for (int index = 0, sock = sck.getSocket(0);sock != -1; sock = sck.getSocket(++index)) {
		eraseReg(sock);
		addReg(sock,desc,request.waitFor);
	}
	if (request.reqNotify) request.reqNotify->wakeUp();

}

void EPollListener::eraseReg(const RefCntPtr<FdDesc>& fdesc) {
	INetworkSocket& sck = fdesc->rsrc->getIfc<INetworkSocket>();
	for (int index = 0, sock = sck.getSocket(0); sock != -1;
			sock = sck.getSocket(++index)) {
		unregFd(epoll_fd, sock);
		socketMap.erase(sock);
	}
}

void EPollListener::eraseTimeout(const RefCntPtr<FdDesc>& fdesc) {
	if (!fdesc->timeout.isInfinite()) {
		natural pos;
		if (fdesc->hintPos != 0) {
			pos = fdesc->hintPos - socketHeapCont.data();
			if (pos > socketHeapCont.getAllocatedSize()) pos = socketHeapCont.find(fdesc);
		} else {
			 pos = socketHeapCont.find(fdesc);
		}
		if (pos >= socketHeapCont.length()) return;
		socketTimeouts.pop(pos);
		socketHeapCont.resize(socketTimeouts.getSize());
	}
}

void EPollListener::eraseReg(int fd) {
	const RefCntPtr<FdDesc> *obj = socketMap.find(fd);
	if (obj) {
		RefCntPtr<FdDesc> fdesc = *obj;

		eraseReg(fdesc);
		eraseTimeout(fdesc);
	}
}

void EPollListener::addReg( int fd, RefCntPtr<FdDesc> fdesc, natural waitFor) {
	socketMap.insert(fd,fdesc);
	regFd(epoll_fd,waitFor,fd,fdesc);
	if (!fdesc->timeout.isInfinite()) {
		socketHeapCont.add(fdesc);
		socketTimeouts.push();
	}
}

EPollListener::FdDesc::FdDesc(RefCntPtr<INetworkResource> rsrc, const Timeout& timeout, ISleepingObject* wk)
	:timeout(timeout),ntf(wk),rsrc(rsrc),hintPos(0) {}

bool EPollListener::TimeoutOrder::operator ()(const RefCntPtr<FdDesc>& a,
		const RefCntPtr<FdDesc>& b) const {
	return a->timeout > b->timeout;
}

} /* namespace LightSpeed */



