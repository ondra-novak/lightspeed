/*
 * NetworkEventListener.cpp
 *
 *  Created on: 13.4.2011
 *      Author: ondra
 */


#include "networkEventListener2.h"
#include "../../mt/exceptions/threadException.h"
#include "../containers/autoArray.tcc"
#include "../exceptions/stdexception.h"
#include "../containers/map.h"
#include "../interface.tcc"
#include "../framework/app.h"

typedef struct epoll_event EPOLL_EVENT;

namespace LightSpeed {

WinNetworkEventListener::WinNetworkEventListener()
{
	enableMTAccess();
	factory = createDefaultNodeAllocator();
}

WinNetworkEventListener::~WinNetworkEventListener() {
	if (worker.isRunning()) {
		worker.finish();
		notify();
		worker.join();
	}

}


void WinNetworkEventListener::RequestMsg::done() throw() {
	if (r.reqNotify)
		r.reqNotify->wakeUp();
	delete this;
}

void WinNetworkEventListener::set(const Request& request) {
	bool r = worker.isRunning();
	if (r && worker.getThreadId() == ThreadId::current()) {
		doRequest(request);
	} else {
		Synchronized<FastLock> _(lock);
		if (!r) {

			try {
				worker.start(ThreadFunction::create(this,&WinNetworkEventListener::workerProc));
			} catch (ThreadBusyException &) {
			}
		}

		RequestMsg *msg = new(reqAlloc) RequestMsg(*this,request);
		postMessage(msg);
	}
}

void WinNetworkEventListener::notify() {
	fdSelect.wakeUp(0);
}




void WinNetworkEventListener::doRequest(const Request& r) {

	INetworkSocket &sockifc = r.rsrc->getIfc<INetworkSocket>();
	SOCKET sck = sockifc.getSocket(0);
	for (int i = 0; sck > -1; sck = sockifc.getSocket(++i)) {
		doRequestWithSocket(r,sck);
	}

}

void WinNetworkEventListener::doRequestWithSocket(const Request &r, SOCKET sck) {
	if (r.waitFor == 0) {

		FdData *dta = reinterpret_cast<FdData *>(fdSelect.getUserData((int)sck));
		if (dta == 0) return; ///< not in map - dummy action

		for (natural i = 0; i < dta->listeners.length(); i++) {
			if (dta->listeners[i].notify == r.notify) {
				dta->listeners.erase(i);
				break;
			}
		}
		updateFdData(dta,(int)sck);


	} else {
		//add to socket
		FdData *dta = reinterpret_cast<FdData *>(fdSelect.getUserData((int)sck));
		AllocPointer<FdData> dta_close;
		if (dta == 0) {
			dta = new(*factory) FdData;
			dta_close = dta;
		}

		bool found = false;
		for (natural i = 0; i < dta->listeners.length(); i++) {
			if (dta->listeners[i].notify == r.notify) {
				dta->listeners(i) = FdListener(r);
				found = true;
				break;
			}
		}
		if (!found) {
			dta->listeners.add(FdListener(r));
		}

		updateFdData(dta,(int)sck);
		dta_close.detach();
	}
}

void WinNetworkEventListener::updateFdData(FdData* dta, int sck) {
	ListenerMap::Iterator iter = dta->listeners.getFwIter();
	if (iter.hasItems()) {
		const FdListener &e = iter.getNext();
		Timeout lowestTm = e.waitTimeout;
		natural waitMask = e.waitMask;
		while (iter.hasItems()) {
			const FdListener &e = iter.getNext();
			Timeout t = e.waitTimeout;
			if (t < lowestTm) lowestTm = t;
			waitMask |= e.waitMask;
		}

		fdSelect.set(sck,waitMask,lowestTm,dta);
	} else {
		fdSelect.unset(sck);
		delete dta;

	}
}

class WinNetworkEventListener::CleanUpProc {
public:
	CleanUpProc(WinNetworkEventListener *owner):owner(owner) {}
	void operator()(int fd, void *userData) const {
		owner->cleanFd(fd, userData);
	}
protected:
	WinNetworkEventListener *owner;
};

void WinNetworkEventListener::workerProc() {

	while (!Thread::canFinish()) {
		try {
			WinSelect::Result res;
			WinSelect::WaitStatus wt = fdSelect.wait(nil, res);

			if (wt == WinSelect::waitWakeUp) {
				while (pumpMessage());
			} else {

				AutoArray<std::pair<ISleepingObject *, natural>,SmallAlloc<32> > tocall;

				SysTime tm  = SysTime::now();
				FdData *listeners = reinterpret_cast<FdData *>(res.userData);
				for (ListenerMap::Iterator iter = listeners->listeners.getFwIter(); iter.hasItems();) {
					const FdListener &l = iter.peek();
					if (res.flags & l.waitMask) {
						tocall.add(std::make_pair(l.notify,res.flags & l.waitMask));
						listeners->listeners.erase(iter);
//						l.notify->wakeUp(con.waitMask & l.waitMask);
					} else if (l.waitTimeout.expired(tm)) {
						tocall.add(std::make_pair(l.notify,0));
						listeners->listeners.erase(iter);
	//					l.notify->wakeUp(0);
					} else {
						iter.skip();
					}
				}
				updateFdData(listeners,res.fd);

				for (natural i = 0; i < tocall.length(); i++) {
					tocall[i].first->wakeUp(tocall[i].second);
				}
			}
		} catch (const Exception &e) {
			AppBase::current().onThreadException(e);
		} catch (const std::exception &e) {
			AppBase::current().onThreadException(StdException(THISLOCATION,e));
		} catch (...) {
			AppBase::current().onThreadException(UnknownException(THISLOCATION));
		}
	}

	fdSelect.cancelAll(CleanUpProc(this));
/*	fdSelect.dropAll();
	while (fdSelect.hasItems()) {
		const LinuxFdSelect::FdInfo &con = fdSelect.getNext();
		FdData *listeners = reinterpret_cast<FdData *>(con.data);
		if (listeners) delete listeners;
		fdSelect.unset(con.fd);
	}
	*/

}



void WinNetworkEventListener::RequestMsg::run() throw() {
	try {
		owner.doRequest(r);
	} catch (const Exception &e) {
		AppBase::current().onThreadException(e);
	} catch (const std::exception &e) {
		AppBase::current().onThreadException(StdException(THISLOCATION,e));
	} catch (...) {
		AppBase::current().onThreadException(UnknownException(THISLOCATION));
	}
}

void WinNetworkEventListener::cleanFd(int , void* data) {
	FdData *listeners = reinterpret_cast<FdData *>(data);
	if (listeners) delete listeners;
}

}


