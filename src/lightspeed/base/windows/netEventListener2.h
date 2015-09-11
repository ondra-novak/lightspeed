#pragma once
#include "..\streams\netio_ifc.h"
#include "..\actions\parallelExecutor.h"
#include "..\containers\map.h"
#include "..\memory\poolalloc.h"
#include "..\actions\msgQueue.h"
#include "..\sync\threadVar.h"
#include "..\memory\smallAlloc.h"


namespace LightSpeed {

	class WindowsNetworkEventListener2 : public INetworkEventListener, private MsgQueue {
	public:


		WindowsNetworkEventListener2();
		~WindowsNetworkEventListener2();
		virtual void set(const Request &request);


	protected:

		const static natural maxWaitEvents;

		ParallelExecutor threadPool;

		class Observer {
		public:
			ISleepingObject *wk;
			Timeout timeout;
			DWORD flags;			

			Observer(ISleepingObject *wk, const Timeout &timeout, DWORD flags)
				:wk(wk), timeout(timeout), flags(flags) {}
		};

		class SocketInfoTmRef;

		class SocketInfo: public RefCntObj, public DynObject {
		public:
			///event that is associated with the socket (WSAEventSelect)
			HANDLE hSockEvent;
			///set true if anything changed in this structure before you call for alter
			Timeout timeout;
			///reference kept during waiting to prevent closing resource outside
			RefCntPtr<INetworkResource> resource;
			///
			DWORD flags;
			///
			AutoArray<Observer, SmallAlloc<4> > observers;

			FastLockR *ownerLock;

			bool deleted;
			bool timeouted;
			bool dirty;

			SocketInfo();
			~SocketInfo();

			void updateObserver(const Observer &observer);

			SocketInfoTmRef *tmRef;
			
		};

		typedef RefCntPtr<SocketInfo> PSocketInfo;

		class SocketInfoTmRef : public PSocketInfo {
		public:
			SocketInfoTmRef(const PSocketInfo &other) :PSocketInfo(other) { get()->tmRef = this; }
			SocketInfoTmRef(const SocketInfoTmRef &other) :PSocketInfo(other) { get()->tmRef = this; }
			SocketInfoTmRef &operator=(const SocketInfoTmRef &other) {
				PSocketInfo::operator=(other);
				get()->tmRef = this;
				return *this;
			}
		};


		struct CmpTimeout {
			bool operator()(const PSocketInfo &a, const PSocketInfo &b) const;
		};


		typedef Map<SOCKET, PSocketInfo> SocketMap;
		typedef AutoArray<HANDLE> EventPool;

		///Map of all sockets
		SocketMap socketMap;
		FastLock lock;
		typedef Synchronized<FastLock> Sync;


		void cancelAll();

		void worker();

		///count of threads ready to pick - if zero, then none of them, we have to create new one
		atomic readyToPick;



		virtual void notify() ;

		PoolAlloc allocPool;

		ParallelExecutor workers;

		HANDLE hPick;

		class RequestWithSocketIndex: public Request {
		public:
			RequestWithSocketIndex(const Request &request, natural index) :Request(request), index(index) {}

		public:
			natural index;
		};

		void onRequest(const RequestWithSocketIndex &request);

		class Helper;
		ThreadVar<Helper> workerContext;

		bool exitting;
	};



}