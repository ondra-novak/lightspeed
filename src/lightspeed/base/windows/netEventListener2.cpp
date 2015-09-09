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
		typedef HeapSort<SockInfoSet, CmpTimeout> TimeoutHeap;

		void eraseSocket( natural index);
		natural addSocket(SOCKET socket, PSocketInfo sinfo);


		SocketSet socketSet;
		SockInfoSet sockInfoSet;
		HandleSet handleSet;
		SockInfoSet timeoutsCont;
		TimeoutHeap timeoutsHeap;
		WindowsNetworkEventListener2 *owner;
		Helper(WindowsNetworkEventListener2 *owner) :owner(owner),timeoutsHeap(timeoutsCont) {}
	};

	void WindowsNetworkEventListener2::worker()
	{

		DbgLog::setThreadName("network", true);
		LS_LOGOBJ(lg);

		lg.debug("Thread created");

		Helper helper(this);
		workerContext.set(ITLSTable::getInstance(), &helper);

		natural havePickHandle = 0;

		do {
			havePickHandle = helper.handleSet.length() ;

			if (havePickHandle < maxWaitEvents) {
				lockInc(readyToPick);
				helper.handleSet.add(hPick);
				lg.debug("Pick used");
			}

			DWORD minTimeout = INFINITE;
			DWORD waitRes;
			if (helper.sockInfoSet.length() == 0) {
				lg.debug("Idle waiting");
					waitRes = WaitForMultipleObjects(helper.handleSet.length(), helper.handleSet.data(), FALSE, 0);
				if (waitRes == WAIT_TIMEOUT) break;
			}
			else {

				DWORD mintimeout = INFINITE;
				if (!helper.timeoutsHeap.empty()) {
					const PSocketInfo &mtm = helper.timeoutsHeap.top();
					mintimeout = mtm->timeout.getRemain().msecs();
				}

				lg.debug("Waiting for events: timeout=%1") << (natural)mintimeout;
				waitRes = WSAWaitForMultipleEvents(helper.handleSet.length(), helper.handleSet.data(), FALSE, mintimeout,FALSE);
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
					helper.timeoutsHeap.pop();
					itm->timeouted = true;
					SetEvent(itm->hSockEvent);
					helper.timeoutsCont.resize(helper.timeoutsHeap.getSize());
				}
			}
			else if (waitRes == havePickHandle) {
				Sync _(lock);
				while (pumpMessage()) {}
			}
			else {
				natural index = waitRes - WAIT_OBJECT_0;

				natural eventNum = naturalNull;

				PSocketInfo itm = helper.sockInfoSet[index];
				if (itm->timeouted) {
					eventNum = 0;
					lg.debug("timeouted: %1") << (natural)itm.get();

				}
				else if (!itm->deleted) {

					//otherwise index points on descriptor with an event
					WSANETWORKEVENTS ev;
					ev.lNetworkEvents = 0;
					//read events
					WSAEnumNetworkEvents(helper.socketSet[index], helper.handleSet[index], &ev);
					//reading
					if (ev.lNetworkEvents & (FD_READ | FD_ACCEPT | FD_CLOSE)) {
						//and writting
						if (ev.lNetworkEvents & (FD_CONNECT | FD_WRITE)) {
							eventNum = INetworkResource::waitForInput | INetworkResource::waitForOutput;
						}
						//reading only
						else {
							eventNum = INetworkResource::waitForInput;
						}
					}
					//writing only
					else if (ev.lNetworkEvents & (FD_CONNECT | FD_WRITE)) {
						eventNum = INetworkResource::waitForOutput;
					}
					lg.debug("event on: %1 - %2") << (natural)itm.get() << eventNum;
				}
				else {
					lg.debug("deleting: %1") << (natural)itm.get();
				}
				if (!itm->timeout.isInfinite()) {
					natural pos = helper.timeoutsCont.find(itm);
					if (pos != naturalNull) {
						helper.timeoutsHeap.pop(pos);
						helper.timeoutsCont.resize(helper.timeoutsHeap.getSize());
					}
				}
				//and finally, erase socket from the pool
				helper.eraseSocket(index);
			
				if (eventNum != naturalNull) {
					//now, we can call the callback
					lg.debug("call handler: %1") << (natural)itm.get();

					itm->wk->wakeUp(eventNum);
				}
			}

		} while (true);

		if (havePickHandle) {
			lockDec(readyToPick);
		}

		lg.debug("Thread exited");
	}

	void WindowsNetworkEventListener2::Helper::eraseSocket(natural index)
	{
		{
			Sync _(owner->lock);
			owner->socketMap.erase(socketSet[index]);
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


	WindowsNetworkEventListener2::SocketInfo::SocketInfo() :deleted(false),timeouted(false)
	{
		hSockEvent = WSACreateEvent();
		LS_LOG.debug("Created socket info: %1 ") << (natural)this;
	}

	WindowsNetworkEventListener2::SocketInfo::~SocketInfo()
	{
		CloseHandle(hSockEvent);
		LS_LOG.debug("Deleted socket info: %1 ") << (natural)this;
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
					PSocketInfo sinfo = *sinfoPtr;
					sinfo->deleted = true;
					WSASetEvent(sinfo->hSockEvent);
				}
				if (request.waitFor != 0) {
					PSocketInfo sinfo = new (allocPool)SocketInfo;
					sinfo = sinfo.getMT();
					sinfo->timeout = request.timeout_ms == naturalNull ? Timeout() : Timeout(request.timeout_ms);
					sinfo->resource = request.rsrc;
					sinfo->wk = request.notify;
					DWORD waitFlags = 0;
					if (request.waitFor & INetworkResource::waitForInput)
						waitFlags |= FD_ACCEPT | FD_READ | FD_CLOSE;
					if (request.waitFor & INetworkResource::waitForOutput)
						waitFlags |= FD_CONNECT | FD_WRITE;
					if (WSAEventSelect(s, sinfo->hSockEvent, waitFlags) == SOCKET_ERROR) {
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