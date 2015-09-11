#include "winpch.h"
#include <winsock2.h>
#include "netEventListener2.h"
#include "..\memory\staticAlloc.h"
#include "..\containers\autoArray.tcc"
#include "..\containers\sort.tcc"
#include "..\interface.tcc"
#include "..\debug\dbglog.h"


namespace LightSpeed {

	const natural WindowsNetworkEventListener2::maxWaitEvents = WSA_MAXIMUM_WAIT_EVENTS;

	static DWORD convertEventFlags(natural waitFor) {
		DWORD waitFlags = 0;
		if (waitFor & INetworkResource::waitForInput)
			waitFlags |= FD_ACCEPT | FD_READ | FD_CLOSE;
		if (waitFor & INetworkResource::waitForOutput)
			waitFlags |= FD_CONNECT | FD_WRITE;
		return waitFlags;
	}

	static natural flagsToLSEvent(DWORD flags)
	{
		natural r = 0;
		if (flags & (FD_ACCEPT | FD_READ | FD_CLOSE)) r |= INetworkResource::waitForInput;
		if (flags & (FD_CONNECT | FD_WRITE)) r |= INetworkResource::waitForOutput;
	}


	WindowsNetworkEventListener2::WindowsNetworkEventListener2() :workers(integerNull,30000,0,1000)
	{
		hPick = CreateEvent(0, 0, 0, 0);
		readyToPick = 0;
		exitting = false;
	}

	WindowsNetworkEventListener2::~WindowsNetworkEventListener2()
	{
		cancelAll();
		CloseHandle(hPick);
	}

	void WindowsNetworkEventListener2::set(const Request &request)
	{
		if (exitting) {
			if (request.reqNotify)request.reqNotify->wakeUp(naturalNull);
			return;
		}
		MsgQueue::execute(Action::create(this, &WindowsNetworkEventListener2::onRequest, RequestWithSocketIndex(request,0)));
	}

	void WindowsNetworkEventListener2::notify()
	{
		SetEvent(hPick);
		if (readAcquire(&readyToPick) == 0) {
			workers.execute(Action::create(this, &WindowsNetworkEventListener2::worker));
		}
	}


	class WindowsNetworkEventListener2::Helper {
	public:
		typedef AutoArray<HANDLE, StaticAlloc<maxWaitEvents> > HandleSet;
		typedef AutoArray<PSocketInfo, StaticAlloc<maxWaitEvents> > SockInfoSet;
		typedef AutoArray<SOCKET, StaticAlloc<maxWaitEvents> > SocketSet;		
		typedef AutoArray<SocketInfoTmRef, StaticAlloc<maxWaitEvents> > TimeoutsCont;
		typedef HeapSort<TimeoutsCont, CmpTimeout> TimeoutHeap;

		bool eraseSocket( natural index, bool whenEmpty);
		natural addSocket(SOCKET socket, PSocketInfo sinfo);
		void updateTmHeapOnDirty(PSocketInfo updatedRef);
		void updateTmHeapOnRemove(PSocketInfo updatedRef);


		SocketSet socketSet;
		SockInfoSet sockInfoSet;
		HandleSet handleSet;
		TimeoutsCont timeoutsCont;
		TimeoutHeap timeoutsHeap;
		WindowsNetworkEventListener2 *owner;
		//prevent changes of this state outside of waiting 
		//only during waiting, the internal state can be updated, because thread is not touching it
		//once the internal state is updated, foreign thread must wake up this thread to apply changes
		FastLockR lock;
		Helper(WindowsNetworkEventListener2 *owner) :owner(owner),timeoutsHeap(timeoutsCont) {}
	};

	void WindowsNetworkEventListener2::worker()
	{

		AutoArray <std::pair<ISleepingObject *, DWORD> > tmpObservers;

		DbgLog::setThreadName("network", true);
		LS_LOGOBJ(lg);

		lg.debug("Thread created");

		Helper helper(this);
		workerContext.set(ITLSTable::getInstance(), &helper);

		Synchronized<FastLockR> _(helper.lock);


		natural havePickHandle = 0;

		do {
			havePickHandle = helper.handleSet.length() ;

			if (havePickHandle < maxWaitEvents) {
				lockInc(readyToPick);
				helper.handleSet.add(hPick);
			}

			DWORD minTimeout = INFINITE;
			DWORD waitRes;
			if (helper.sockInfoSet.length() == 0) {
				lg.debug("Idle waiting");
				SyncReleased<FastLockR> _(helper.lock);
					waitRes = WaitForMultipleObjects(helper.handleSet.length(), helper.handleSet.data(), FALSE, 100);
				if (waitRes == WAIT_TIMEOUT) break;
			}
			else {

				DWORD mintimeout = INFINITE;
				if (!helper.timeoutsHeap.empty()) {
					const PSocketInfo &mtm = helper.timeoutsHeap.top();
					mintimeout = mtm->timeout.getRemain().msecs();
				}

				lg.debug("Waiting for events: timeout=%1") << (natural)mintimeout;
				SyncReleased<FastLockR> _(helper.lock);
				waitRes = WSAWaitForMultipleEvents(helper.handleSet.length(), helper.handleSet.data(), FALSE, mintimeout, FALSE);
			}
			if (havePickHandle < maxWaitEvents) {
				lockDec(readyToPick);
				helper.handleSet.resize(helper.sockInfoSet.length());
			}
			if (waitRes == WAIT_TIMEOUT) {
				SysTime now = SysTime::now();
				bool exp = false;
				while (!helper.timeoutsHeap.empty() && helper.timeoutsHeap.top()->timeout.expired(now)) {
					exp = true;
					PSocketInfo itm = helper.timeoutsHeap.top();
					lg.debug("timeouted: %1") << (natural)itm.get();
					itm->timeouted = true;
					SetEvent(itm->hSockEvent);
					helper.timeoutsHeap.pop();
					helper.timeoutsCont.trunc(1);
					itm->tmRef = 0; //< we removed timeout from top, so reset tmRef to prevent mistakenly update timeoutsHeap
				}
			}
			else if (waitRes == havePickHandle) {
				SyncReleased<FastLockR> __(helper.lock);
				Sync _(lock);
				while (pumpMessage()) {
					if (helper.sockInfoSet.length() == maxWaitEvents) 
						break;
				}
			}
			else {
				natural index = waitRes - WAIT_OBJECT_0;

				natural eventNum = naturalNull;

				PSocketInfo itm = helper.sockInfoSet[index];
				if (itm->timeouted) {
					for (natural i = 0; i < itm->observers.length();) {
						if (itm->observers[i].timeout.expired()) {
							tmpObservers.add(std::make_pair(itm->observers[i].wk, DWORD(0)));
							itm->observers.erase(i);
						}
						else {
							i++;
						}
					}
					for (natural i = 0; i < tmpObservers.length(); i++) {
						tmpObservers[i].first->wakeUp(0);
					}
					tmpObservers.clear();
					if (itm->observers.empty()) {
						//remove timeout from heap as well
						helper.eraseSocket(index,true);
					}
				}
				else if (itm->deleted) {
					//remove timeout from heap as well					
					helper.eraseSocket(index,false);
				}
				else {

					PSocketInfo itm = helper.sockInfoSet[index];

					//otherwise index points on descriptor with an event
					WSANETWORKEVENTS ev;
					ev.lNetworkEvents = 0;
					//read events
					WSAEnumNetworkEvents(helper.socketSet[index], helper.handleSet[index], &ev);

					for (natural i = 0; i < itm->observers.length();) {
						const Observer &o = itm->observers[i];
						if (o.flags & ev.lNetworkEvents) {
							tmpObservers.add(std::make_pair(o.wk, o.flags & ev.lNetworkEvents));
							itm->observers.erase(i);
						}
						else {
							i++;
						}
					}
					for (natural i = 0; i < tmpObservers.length(); i++) {
						tmpObservers[i].first->wakeUp(tmpObservers[i].second);
					}
					tmpObservers.clear();
					helper.eraseSocket(index,true);
				}
			}
		} while (true);

		if (havePickHandle < maxWaitEvents) {
			lockDec(readyToPick);
		}

		lg.debug("Thread exited");
	}

	bool WindowsNetworkEventListener2::Helper::eraseSocket(natural index, bool whenEmpty)
	{
		{
			SyncReleased<FastLockR> __(lock);
			Sync _(owner->lock);
			if (whenEmpty && !sockInfoSet[index]->observers.empty()) return false;
			owner->socketMap.erase(socketSet[index]);
		}
		if (!sockInfoSet[index]->timeout.isInfinite()) {
			updateTmHeapOnRemove(sockInfoSet[index]);
		}
		natural last = sockInfoSet.length() - 1;
		if (last != index) {
			handleSet(index) = handleSet[last];
			socketSet(index) = socketSet[last];
			sockInfoSet(index) = sockInfoSet[last];
		}
		handleSet.resize(last);
		socketSet.resize(last);
		sockInfoSet.resize(last);
		return true;
	}

	natural WindowsNetworkEventListener2::Helper::addSocket(SOCKET socket, PSocketInfo sinfo)
	{
		natural index = sockInfoSet.length();
		handleSet.resize(index);
		socketSet.resize(index);
		sockInfoSet.add(sinfo);
		socketSet.add(socket);
		handleSet.add(sinfo->hSockEvent);
		return index;

	}

	void WindowsNetworkEventListener2::Helper::updateTmHeapOnDirty(PSocketInfo updatedRef)
	{
		if (updatedRef->tmRef == 0) {
			timeoutsCont.add(updatedRef);
			timeoutsHeap.push();
		}
		natural index = updatedRef->tmRef - timeoutsCont.data();
		if (index >= timeoutsCont.length()) {
			timeoutsHeap.makeHeap();
		}
		else {
			timeoutsHeap.pop(index);
			timeoutsHeap.push();
		}
	}

	void WindowsNetworkEventListener2::Helper::updateTmHeapOnRemove(PSocketInfo updatedRef)
	{
		if (updatedRef->tmRef == 0) return;
		natural index = updatedRef->tmRef - timeoutsCont.data();
		if (index >= timeoutsCont.length()) {
			index = timeoutsCont.find(updatedRef);
			if (index >= timeoutsCont.length())
				return;
		}
		else {
			timeoutsHeap.pop(index);
			timeoutsCont.trunc(1);
			updatedRef->tmRef = 0;
		}
	}

	void WindowsNetworkEventListener2::cancelAll()
	{
		do
		{
			Sync _(lock);			
			for (SocketMap::Iterator iter = socketMap.getFwIter(); iter.hasItems();) {
				PSocketInfo s = iter.getNext().value;
				s->deleted = true;
				WSASetEvent(s->hSockEvent);
			}
		} while (workers.stopAll(100) == false);

		

	}

	bool WindowsNetworkEventListener2::CmpTimeout::operator()(const PSocketInfo &a, const PSocketInfo &b) const
	{
		return a->timeout > b->timeout;
	}


	WindowsNetworkEventListener2::SocketInfo::SocketInfo() :deleted(false), timeouted(false), tmRef(0)
	{
		hSockEvent = WSACreateEvent();
		LS_LOG.debug("Created socket info: %1 ") << (natural)this;
	}

	WindowsNetworkEventListener2::SocketInfo::~SocketInfo()
	{
		CloseHandle(hSockEvent);
		LS_LOG.debug("Deleted socket info: %1 ") << (natural)this;
	}

	void WindowsNetworkEventListener2::SocketInfo::updateObserver(const Observer &observer)
	{
		Timeout minTm;
		DWORD flags = 0;
		natural l = observers.length();
		for (natural i = 0; i < l; i++) {
			if (observers[i].wk == observer.wk) {
				if (i + 1 < l) std::swap(observers(i), observers(l - 1));
				observers.trunc(1);
				l--;
			}
			else {
				if (observers[i].timeout < minTm) minTm = observers[i].timeout;
				flags |= observers[i].flags;
			}
		}
		if (observer.flags) {
			observers.add(observer);
			flags |= observer.flags;
			if (observer.timeout < minTm) minTm = observer.timeout;
		}
		timeout = minTm;
		this->flags = flags;
	}


	void WindowsNetworkEventListener2::onRequest(const RequestWithSocketIndex &request)
	{
		if (exitting) {
			if (request.reqNotify)request.reqNotify->wakeUp(naturalNull);
			return;
		}
		DbgLog::setThreadName("network", true);

		ITLSTable &tls = ITLSTable::getInstance();
		Helper *helper = workerContext[tls];

		natural i = request.index;
		INetworkSocket &ss = request.rsrc->getIfc<INetworkSocket>();
		SOCKET s;
		while ((s = ss.getSocket(i)) != -1) {

			if (request.waitFor == 0 || helper->sockInfoSet.length() < maxWaitEvents) {


				const PSocketInfo *sinfoPtr = socketMap.find(s);
				if (sinfoPtr) {

					PSocketInfo si = *sinfoPtr;
					Synchronized<FastLockR> _(*si->ownerLock);
					si->updateObserver(Observer(request.notify, Timeout(request.timeout_ms), convertEventFlags(request.waitFor)));
					if (si->flags) {
						if (WSAEventSelect(s, si->hSockEvent, si->flags) == SOCKET_ERROR) {
							throw ErrNoException(THISLOCATION, GetLastError());
						}
					}
					else {
						si->deleted = true;
						WSASetEvent(si->hSockEvent);
					}
				} else if (request.waitFor != 0) {
					PSocketInfo sinfo = new (allocPool)SocketInfo;
					sinfo = sinfo.getMT();					
					sinfo->ownerLock = &helper->lock;
					sinfo->updateObserver(Observer(request.notify, Timeout(request.timeout_ms), convertEventFlags(request.waitFor)));

					if (WSAEventSelect(s, sinfo->hSockEvent, sinfo->flags) == SOCKET_ERROR) {
						throw ErrNoException(THISLOCATION, GetLastError());
					}

					LS_LOG.debug("onRequest: socket=%1,index=%2, desc=%3") << s << i << (natural)sinfo.get();
					helper->addSocket(s, sinfo);
					socketMap.insert(s,sinfo);					
					if (!sinfo->timeout.isInfinite())  {
						helper->timeoutsCont.add(sinfo);
						helper->timeoutsHeap.push();
					}
				}
				i++;
			}
			else {
				if (exitting) {
					if (request.reqNotify)request.reqNotify->wakeUp(naturalNull);
					return;
				}
				MsgQueue::execute(Action::create(this, &WindowsNetworkEventListener2::onRequest, RequestWithSocketIndex(request, i)));
				return;
			}
		}
		if (request.reqNotify) request.reqNotify->wakeUp(0);
	}

}