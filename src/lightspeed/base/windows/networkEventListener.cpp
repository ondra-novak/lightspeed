/*
 * NetworkEventListener.cpp
 *
 *  Created on: 13.4.2011
 *      Author: ondra
 */
#include "winpch.h"
#include "networkEventListener.h"
#include "..\framework\iapp.h"
#include "..\exceptions\stdexception.h"
#include "netStream.h"
#include "..\..\mt\exceptions\threadException.h"
#include "..\exceptions\netExceptions.h"
#include "..\memory\staticAlloc.h"
#include "../containers/autoArray.tcc"
#include "winsocket.tcc"
static const int tmEvent = 95;


namespace LightSpeed {

#define WM_SOCKETEVENT (WM_APP+2)
#define WM_WORKERQUIT (WM_APP+3)
#define WM_QUEUEREQUEST (WM_APP+4)

WindowsNetworkEventListener::WindowsNetworkEventListener()
:allocPtr(&alloc), eventMap(allocPtr),messageWnd(0),deferUpdateTimer(0),msgQueue(*this,*allocPtr)
{

}


class WindowsNetworkEventListener::MsgRequest: public MsgQueue::NotifyMsg {
public:
	MsgRequest(WindowsNetworkEventListener &owner, const INetworkEventListener::Request &request)
		:owner(owner),request(request) {}
	virtual void run() throw() {
		owner.registerHandler(request);
	}
protected:

	WindowsNetworkEventListener &owner;
	const INetworkEventListener::Request &request;
};



void WindowsNetworkEventListener::set( const Request &request )
{
	if (&worker == &Thread::current()) {
		registerHandler(request);
	} else if (request.waitFor == 0) {
		MsgRequest rq(*this,request);
		msgQueue.postMessage(&rq);
		while (!rq) Thread::sleep(nil);
	} else {
		msgQueue.postFnCall(
			Message<>::create(
				this,&WindowsNetworkEventListener::registerHandler,request));
	}


}

void WindowsNetworkEventListener::onRequest()
{
	//lock operation - to prevent thread to flee
	Synchronized<FastLock> _(workerQueueLock);
	//is there some window to send message?
	if (messageWnd == 0) {
		//join worker, it may still running in its final phase
		worker.join();
		//initialize master thread
		Thread::getMaster();
		//initialize startup notifier
		Notifier ntf;
		//start thread
		worker.start(ThreadFunction::create(
			this,&WindowsNetworkEventListener::workerProc,&ntf));
		//wait until thread is fully initialized
		ntf.wait(nil);
	}
	//if we still have no window for queue
	if (messageWnd == 0) {
		//join possible failed worker
		worker.join();
		//thown an exception
		throw NetworkIOError(THISLOCATION,0,"Unable to create network event listener");
	}
	//everything ok, post message now
	PostMessage(messageWnd,WM_QUEUEREQUEST,0,0);
}

void WindowsNetworkEventListener::workerProc( Notifier *startNtf )
{
	//define window name for event listener
	const wchar_t *name = L"Lightspeed::WindowsNetworkEventListener";
	//retrieve some hInstance
	HINSTANCE hInst = GetModuleHandle(0);

	try {
		WNDCLASSW cls;
		//check or define window class
		if (GetClassInfoW(hInst,name,&cls) == FALSE) {
			initStruct(cls);
			cls.lpszClassName = name;
			//winproc
			cls.lpfnWndProc = &WindowsNetworkEventListener::staticWndProc;
			cls.hInstance = hInst;
			BOOL res = RegisterClassW(&cls);
			if (res == FALSE) 
				throw ErrNoException(THISLOCATION,GetLastError());
		}
		//create window - use this as lparam
		messageWnd = CreateWindowW(name,L"",0,0,0,0,0,HWND_MESSAGE,0,hInst,this);
		//if creation failed, throw an exception
		if (messageWnd == NULL) {
			throw ErrNoException(THISLOCATION,GetLastError());
		}
		//we have a window, release waiting thread
		startNtf->wakeUp(0);
		
	} catch (...) {
		//reset any possible garbage in messageWnd value
		messageWnd = 0;
		//release waiting thread - it will able to find, that creation failed
		startNtf->wakeUp(0);
		//rethrow
		throw;
	}
	
	//lock posting to allow starting thread finish its post
	Synchronized<FastLock> _(workerQueueLock);
	do {
		//unlock it in cycle
		SyncReleased<FastLock> _(workerQueueLock);
		MSG msg;
		//pick all messages
		while (!Thread::canFinish() && GetMessageAndHandlePipes(&msg)) {
			msgQueue.pumpAllMessages();
			DispatchMessage(&msg);
/*			if (eventMap.empty() && pipeMap.empty()) {
				//if eventMap is empty, wait 1 second for any message and then exit
				MsgWaitForMultipleObjects(0,0,FALSE,1000,QS_ALLPOSTMESSAGE);				
			}*/
		}
		//exit: lock posting - nobody is able to post now
		//check, whether queue is really empty - if not, restart cycle
	} while (!msgQueue.empty() && !Thread::canFinish());
	//nothing more to do - destroy window
	DestroyWindow(messageWnd);
	//reset messageWnd
	messageWnd = 0;
	//unlock posting - waiting thread will now realize that worker gone away
}

WindowsNetworkEventListener::~WindowsNetworkEventListener()
{
	PostMessage(messageWnd,WM_WORKERQUIT,0,0);
	worker.stop();
	for (EventMap::Iterator iter = eventMap.getFwIter(); iter.hasItems();) {
		const EventMap::Entity &e = iter.getNext();
		for (int i = 0; i < 3; i++) {
			if (e.value.waitEvents[i].listener != 0)
				e.value.waitEvents[i].listener->wakeUp(naturalNull);
		}
	}
}

LRESULT WINAPI WindowsNetworkEventListener::staticWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	LONG_PTR x = GetWindowLongPtr(hWnd,GWLP_USERDATA);
	if (x != 0) {
		try {
			WindowsNetworkEventListener *ptr = (WindowsNetworkEventListener *)x;
			return ptr->winProc(msg,wParam,lParam);
		} catch (const Exception &e) {
			IApp *app = IApp::currentPtr();
			if (app) app->onThreadException(e);
			return DefWindowProc(hWnd,msg,wParam,lParam);
		} catch (const std::exception &e) {
			IApp *app = IApp::currentPtr();
			if (app) app->onThreadException(StdException(THISLOCATION,e));
			return DefWindowProc(hWnd,msg,wParam,lParam);
		} catch (...) {
			IApp *app = IApp::currentPtr();
			if (app) app->onThreadException(UnknownException(THISLOCATION));
			return DefWindowProc(hWnd,msg,wParam,lParam);
		}
	} else {
		if (msg == WM_CREATE) {
			CREATESTRUCT *cr = (CREATESTRUCT *)lParam;
			SetWindowLongPtr(hWnd,GWLP_USERDATA,(LONG_PTR)cr->lpCreateParams);
		}
		return DefWindowProc(hWnd,msg,wParam,lParam);
	}
}

LRESULT WINAPI WindowsNetworkEventListener::winProc( UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch (msg) {
		case WM_SOCKETEVENT: socketEvent(wParam, LOWORD(lParam),HIWORD(lParam));break;
		case WM_TIMER: updateTimer();break;
		case WM_WORKERQUIT: PostQuitMessage(0);break;
		default: DefWindowProcW(messageWnd,msg,wParam,lParam);
	}
	return 0;
}


void WindowsNetworkEventListener::socketEvent( SOCKET s, int event, int error )
{
	deferUpdateTimer++;
	try {
		EventInfo *evInfo = eventMap.find(s);
		if (evInfo != 0) {
			if (event & (FD_READ|FD_ACCEPT|FD_CLOSE)) 
				callEvent(s,evInfo,0,INetworkResource::waitForInput);	
			if (event & (FD_WRITE|FD_CONNECT)) 
				callEvent(s,evInfo,1,INetworkResource::waitForOutput);	
			if (event & (FD_OOB)) {
				callEvent(s,evInfo,2,INetworkResource::waitForException);	
			}
			updateEvents(s,evInfo);
		} else {
			WSAAsyncSelect(s,messageWnd,0,0);		
		}
	} catch (...) {
		deferUpdateTimer--;
		throw;
	}
	deferUpdateTimer--;
	updateTimer();
}

void WindowsNetworkEventListener::registerHandler( const Request &r )
{
	INetworkSocket *sockifc = dynamic_cast<INetworkSocket *>(r.rsrc);
	if (NULL != sockifc)
	{
		SOCKET s = sockifc->getSocket(0);
		if (r.waitFor == 0) {
			eventMap.erase(s);
			EventInfo nfo;
			updateEvents(s,&nfo);
		} else {
			EventInfo &hndl = eventMap(s);
			if (r.waitFor & INetworkResource::waitForInput) 
				hndl.waitEvents[0] = EventHandler(r.timeout_ms,r.notify);
			if (r.waitFor & INetworkResource::waitForOutput) 
				hndl.waitEvents[1] = EventHandler(r.timeout_ms,r.notify);
			if (r.waitFor & INetworkResource::waitForException) 
				hndl.waitEvents[2] = EventHandler(r.timeout_ms,r.notify);
			updateEvents(s,&hndl);
		}
	} else {
		IWinWaitableObject *pipeifc = dynamic_cast<IWinWaitableObject *>(r.rsrc);
		if (pipeifc) {
			if (r.waitFor == 0) {
				pipeMap.erase(pipeifc);
			} else {
				PipeInfo nfo;
				nfo.handler = EventHandler(r.timeout_ms,r.notify);
				nfo.waitOp = r.waitFor;
				pipeMap.insert(pipeifc,nfo);
			}
		}
	}
	
	updateTimer();
	if (r.reqNotify) 
		r.reqNotify->wakeUp();
}

DWORD WindowsNetworkEventListener::checkTimeout()
{
	DWORD maxTimeout = INFINITE;
	SysTime timestamp = SysTime::now();
	for (EventMap::Iterator iter = eventMap.getFwIter(); iter.hasItems();) {
		bool canErase = true;
		const EventMap::Entity &e = iter.peek();
		for (int i = 0; i < 3; i++) {
			if (e.value.waitEvents[i].listener != 0) {
				if (e.value.waitEvents[i].timeout.expired(timestamp))
					e.value.waitEvents[i].listener->wakeUp(0);
				else {
					canErase = false;
					DWORD tm = DWORD(e.value.waitEvents[i].timeout.getRemain(timestamp).msecs());
					if (tm < maxTimeout)
						maxTimeout = tm;
				}
			}
		}
		if (canErase)
			eventMap.erase(iter);
		else
			iter.skip();
	}
	for (PipeMap::Iterator iter = pipeMap.getFwIter(); iter.hasItems();) {
		bool canErase = true;
		const PipeMap::Entity &e = iter.getNext();
		if (e.value.handler.timeout.expired(timestamp)) {
			e.value.handler.listener->wakeUp(0);
		} else {
			DWORD tm = DWORD(e.value.handler.timeout.getRemain(timestamp).msecs());
			if (tm < maxTimeout)
				maxTimeout = tm;
		}		
	}
	return maxTimeout;
}

void WindowsNetworkEventListener::updateTimer()
{
	if (deferUpdateTimer) return;
	DWORD tm = checkTimeout();
	while (tm == 0) tm = checkTimeout();

	SetTimer(messageWnd,tmEvent,tm,0);
}

void WindowsNetworkEventListener::updateEvents( SOCKET s, const EventInfo *hndl)
{
	if (hndl == 0) hndl = eventMap.find(s);
	if (hndl == 0) return;
	int events = 0;	
	if (hndl->waitEvents[0].listener)
		events |=FD_READ |FD_ACCEPT |FD_CLOSE;
	if (hndl->waitEvents[1].listener)
		events |=FD_WRITE  |FD_CONNECT;
	if (hndl->waitEvents[2].listener)
		events |=FD_OOB;
	if (events)
		WSAAsyncSelect(s,messageWnd,WM_SOCKETEVENT,events);
	else {
		WSAAsyncSelect(s,messageWnd,0,0);
		eventMap.erase(s);
		updateTimer();
	}
}

void WindowsNetworkEventListener::callEvent(SOCKET s, EventInfo * evInfo, int id, int waitEvent )
{
	WindowsSocketResource<INetworkResource> rsrc(s,0,true);
	if (rsrc.wait(waitEvent,0)) {
		ISleepingObject *o = evInfo->waitEvents[id].listener;
		if (o == 0) return;
		evInfo->waitEvents[id].listener = 0;
		o->wakeUp(waitEvent);
	}
}

BOOL WindowsNetworkEventListener::GetMessageAndHandlePipes( MSG* msg )
{
	do  {
		if (pipeMap.empty()) return GetMessage(msg,0,0,0);
		
		if (PeekMessage(msg,0,0,0,PM_REMOVE)) {
			return msg->message != WM_QUIT;
		}
		typedef std::pair<IWinWaitableObject *,natural> HandleIdent;
		AutoArray<HANDLE, StaticAlloc<MAXIMUM_WAIT_OBJECTS > > handleMap;
		AutoArray<HandleIdent, StaticAlloc<MAXIMUM_WAIT_OBJECTS > > handleIdentMap;

		for (PipeMap::Iterator iter = pipeMap.getFwIter(); iter.hasItems() && handleMap.length() < MAXIMUM_WAIT_OBJECTS -2 ;) {
			const PipeMap::Entity &e = iter.getNext();
			if (e.value.waitOp & INetworkResource::waitForInput) {
				HANDLE h1 = e.key->getWaitHandle(INetworkResource::waitForInput);
				handleMap.add(h1);
				handleIdentMap.add(HandleIdent(e.key,INetworkResource::waitForInput));
			}
			if (e.value.waitOp & INetworkResource::waitForOutput) {
				HANDLE h1 = e.key->getWaitHandle(INetworkResource::waitForOutput);
				handleMap.add(h1);
				handleIdentMap.add(HandleIdent(e.key,INetworkResource::waitForOutput));
			}		
		}
		DWORD res = MsgWaitForMultipleObjects(handleMap.length(),handleMap.data(),FALSE,INFINITE,QS_ALLEVENTS);
		if (res >= WAIT_OBJECT_0 + handleMap.length()) {
			return GetMessage(msg,0,0,0);
		}
		res = res - WAIT_OBJECT_0;
		IWinWaitableObject *wobj = handleIdentMap[res].first;
		natural op = handleIdentMap[res].second;
		if (wobj->onWaitSuccess(op)) {
			ISleepingObject *listener = pipeMap[wobj].handler.listener;
			pipeMap.erase(wobj);
			listener->wakeUp(op);
		}
	}while (true);
}





}


