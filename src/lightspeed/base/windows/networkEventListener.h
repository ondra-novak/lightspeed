/*
 * NetworkEventListener.h
 *
 *  Created on: 13.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_WINDOWS_NETWORKEVENTLISTENER_H_
#define LIGHTSPEED_WINDOWS_NETWORKEVENTLISTENER_H_

#include <WinSock2.h>
#include "..\streams\netio_ifc.h"
#include "..\..\mt\timeout.h"
#include "..\containers\map.h"
#include "..\..\mt\thread.h"
#include "..\..\mt\notifier.h"
#include "..\actions\msgQueue.h"
#include "..\memory\poolAlloc.h"
#include "..\memory\rtAlloc.h"
#include "IWinWaitableObject.h"


namespace LightSpeed {
class WindowsNetworkEventListener: public INetworkEventListener {
public:
	WindowsNetworkEventListener();
	virtual ~WindowsNetworkEventListener();

	virtual void set(const Request &request);


protected:

	struct EventHandler{
		Timeout timeout;
		ISleepingObject *listener;

		EventHandler(Timeout timeout,ISleepingObject *listener)
			:timeout(timeout),listener(listener) {}
		EventHandler():timeout(nil),listener(0) {}
	};

	struct EventInfo {
		EventHandler waitEvents[3];
	};


	struct PipeInfo {
		natural waitOp;		
		EventHandler handler;
	};


	typedef Map<SOCKET, EventInfo, std::less<SOCKET> > EventMap;
	typedef Map<IWinWaitableObject *, PipeInfo> PipeMap;

	class Queue: public MsgQueue {
	public:
		Queue(WindowsNetworkEventListener &owner,IRuntimeAlloc &alloc):MsgQueue(alloc),owner(owner) {}
		virtual void notify() {owner.onRequest();}
		void pumpAllMessages() {
			while (pumpMessage()) {}
		}
	protected:
		WindowsNetworkEventListener &owner;
	};

	class MsgRequest;
	class MsgRequestDelayed;
protected:
	PoolAlloc alloc;
	SharedStaticPtr<IRuntimeAlloc> allocPtr;
	EventMap eventMap;
	PipeMap pipeMap;
	HWND messageWnd;
	Thread worker;
	FastLock workerQueueLock;
	DWORD deferUpdateTimer;
	Queue msgQueue;
	
	void workerProc(Notifier *startNtf);
	static LRESULT WINAPI staticWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT WINAPI winProc( UINT msg, WPARAM wParam, LPARAM lParam );
	DWORD checkTimeout();	
	void socketEvent( SOCKET s, int event, int error );

	void callEvent(SOCKET s, EventInfo * evInfo, int id, int waitEvent );
	void registerHandler( const Request &r );
	void updateTimer();
	void updateEvents(SOCKET s, const EventInfo *hndl);

	void onRequest();
	BOOL GetMessageAndHandlePipes( MSG* msg );

};

}
#endif /* LIGHTSPEED_WINDOWS_NETWORKEVENTLISTENER_H_ */
