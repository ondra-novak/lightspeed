/*
 * NetworkEventListener.cpp
 *
 *  Created on: 13.4.2011
 *      Author: ondra
 */

#include <unistd.h>
#include "networkEventListener.h"
#include <fcntl.h>
#include <sys/epoll.h>
#include "../../mt/exceptions/threadException.h"
#include "../containers/autoArray.tcc"
#include "../exceptions/stdexception.h"
#include "../containers/map.h"
#include "../interface.tcc"
#include "../framework/app.h"

typedef struct epoll_event EPOLL_EVENT;

namespace LightSpeed {

LinuxNetworkEventListener::LinuxNetworkEventListener()
{
	enableMTAccess();
	factory = createDefaultNodeAllocator();
}

LinuxNetworkEventListener::~LinuxNetworkEventListener() {
	if (worker.isRunning()) {
		worker.finish();
		notify();
		worker.join();
	}

}


void LinuxNetworkEventListener::RequestMsg::done() throw() {
	if (r.reqNotify)
		r.reqNotify->wakeUp();
	delete this;
}

void LinuxNetworkEventListener::set(const Request& request) {
	bool r = worker.isRunning();
	if (r && worker.getThreadId() == ThreadId::current()) {
		doRequest(request);
	} else {
		Synchronized<FastLock> _(lock);
		if (!r) {

			fdSelect.enableWakeUp(true);
			try {
				worker.start(ThreadFunction::create(this,&LinuxNetworkEventListener::workerProc));
			} catch (ThreadBusyException &e) {
			}
		}

		RequestMsg *msg = new(reqAlloc) RequestMsg(*this,request);
		postMessage(msg);
	}
}

void LinuxNetworkEventListener::notify() {
	fdSelect.wakeUp(0);
}




void LinuxNetworkEventListener::doRequest(const Request& r) {

	INetworkSocket &sockifc = r.rsrc->getIfc<INetworkSocket>();
	for (int i = 0, sck = sockifc.getSocket(0); sck > -1; sck = sockifc.getSocket(++i)) {
		doRequestWithSocket(r,sck);
	}

}

void LinuxNetworkEventListener::doRequestWithSocket(const Request &r, int sck) {
	if (r.waitFor == 0) {

		FdData *dta = reinterpret_cast<FdData *>(fdSelect.getData(sck));
		if (dta == 0) return; ///< not in map - dummy action

		for (natural i = 0; i < dta->listeners.length(); i++) {
			if (dta->listeners[i].notify == r.notify) {
				dta->listeners.erase(i);
				break;
			}
		}
		updateFdData(dta,sck);


	} else {
		//add to socket
		FdData *dta = reinterpret_cast<FdData *>(fdSelect.getData(sck));
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

		updateFdData(dta,sck);
		dta_close.detach();
	}
}

void LinuxNetworkEventListener::updateFdData(FdData* dta, int sck) {
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

		fdSelect.set(sck,waitMask,dta,lowestTm);
	} else {
		fdSelect.unset(sck);
		delete dta;

	}
}

void LinuxNetworkEventListener::workerProc() {

	while (!Thread::canFinish()) {
		try {
			const LinuxFdSelect::FdInfo &con = fdSelect.getNext();
			natural reason;
			if (fdSelect.isWakeUpRequest(con,reason)) {
				while (pumpMessage());
			} else {

				AutoArray<std::pair<ISleepingObject *, natural>,SmallAlloc<32> > tocall;

				SysTime tm  = SysTime::now();
				FdData *listeners = reinterpret_cast<FdData *>(con.data);
				for (ListenerMap::Iterator iter = listeners->listeners.getFwIter(); iter.hasItems();) {
					const FdListener &l = iter.peek();
					if (con.waitMask & l.waitMask) {
						tocall.add(std::make_pair(l.notify,con.waitMask & l.waitMask));
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
				updateFdData(listeners,con.fd);

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

	fdSelect.dropAll();
	while (fdSelect.hasItems()) {
		const LinuxFdSelect::FdInfo &con = fdSelect.getNext();
		FdData *listeners = reinterpret_cast<FdData *>(con.data);
		if (listeners) delete listeners;
		fdSelect.unset(con.fd);
	}

}


void LinuxNetworkEventListener::RequestMsg::run() throw() {
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

}


