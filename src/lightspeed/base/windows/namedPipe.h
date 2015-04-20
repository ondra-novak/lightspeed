#pragma once
#include "..\streams\netio_ifc.h"
#include "fileio.h"
#include "IWinWaitableObject.h"

namespace LightSpeed {



	class NamedPipeServerAddr: public INetworkAddress {
	public:
		virtual StringA asString(bool resolve = false);
		virtual bool equalTo(const INetworkAddress &other) const;
		virtual bool lessThan(const INetworkAddress &other) const;
		virtual natural getSockAddress(void *buffer, natural size) const ;

		NamedPipeServerAddr(String pipeName);

	protected:
		String pipeName;
	};

	class ISecurityAttributes {
	public:
		virtual LPSECURITY_ATTRIBUTES getSecurityAttributes() const = 0;
		virtual ~ISecurityAttributes() {}
	};


	class NamedPipeServer: public NetworkResourceCommon<INetworkStreamSource>, public IWinWaitableObject {
	public:
		virtual bool hasItems() const ;
		virtual PNetworkStream getNext() ;
		virtual PNetworkAddress getPeerAddr() const;
		virtual PNetworkAddress getLocalAddr() const ;
		virtual HANDLE getWaitableObject(natural waitFor) const ;

		NamedPipeServer(
				ConstStrW lpName,
				DWORD dwOpenMode,
				DWORD dwPipeMode,
				DWORD nMaxInstances,
				DWORD nOutBufferSize,
				DWORD nInBufferSize,
				natural connectTimeout,
				natural streamTimeout,
				ISecurityAttributes *secAttrs = 0);

		~NamedPipeServer();
		void preparePipe();

		virtual natural getDefaultWait() const;

		virtual void setTimeout( natural time_in_ms );

		virtual natural getTimeout() const;

		virtual HANDLE getWaitHandle(natural waitFor);


		virtual bool onWaitSuccess( natural waitFor );

	protected:
		String lpName;
		DWORD dwOpenMode;
		DWORD dwPipeMode;
		DWORD nMaxInstances;
		DWORD nOutBufferSize;
		DWORD nInBufferSize;
		AllocPointer<ISecurityAttributes> secAttrs;
		OVERLAPPED ovr;
		HANDLE hPrepared;
		HANDLE hEvent;
		PException storedExcept;
		Pointer<WaitHandler> waitHandler;
		natural connectTimeout;
		natural streamTimeout;

		virtual natural doWait( natural waitFor, natural timeout ) const;

	};	


	class NamedPipe: public NetworkResourceCommon<INetworkStream>, public IWinWaitableObject {
	public:

		NamedPipe(String name, HANDLE hPipe, bool allowRead, bool allowWrite);
		~NamedPipe();

		virtual natural read( void *buffer, natural size );

		virtual natural write( const void *buffer, natural size );

		virtual natural peek( void *buffer, natural size ) const;

		virtual bool canRead() const;

		virtual bool canWrite() const;

		virtual void flush();

		virtual natural dataReady() const;

		virtual void closeOutput();

		virtual natural getDefaultWait() const;

		virtual HANDLE getWaitHandle(natural waitFor);

		virtual bool onWaitSuccess(natural waitFor) ;

	protected:
		HANDLE hPipe;
		HANDLE hReadWait;
		HANDLE hWriteWait;
		mutable OVERLAPPED ovrRead;
		mutable OVERLAPPED ovrWrite;
		mutable byte readBuff[256];
		mutable byte writeBuff[256];
		mutable natural rdBuffUse;
		mutable natural wrBuffUse;	
		mutable bool readCharged;
		mutable bool writeCharged;
		FastLock lk;
		String name;
		bool eof;
		bool allowWrite;		

		void internalRead() ;
		bool finishRead();
		void internalWrite();

		natural internalWait( natural waitFor, natural timeout );
		bool finishWrite();

		virtual natural doWait(natural waitFor, natural timeout) const;

	};


	class NamedPipeClient:public NetworkResourceCommon<INetworkStreamSource>, public IWinWaitableObject {
	public:
		NamedPipeClient(String name, DWORD openMode, natural maxInstances, natural streamDefTimeout);
		~NamedPipeClient();

		virtual bool hasItems() const;

		virtual PNetworkStream getNext();

		void prepare();

		virtual PNetworkAddress getPeerAddr() const;

		virtual PNetworkAddress getLocalAddr() const;

		virtual natural getDefaultWait() const;

		virtual HANDLE getWaitHandle( natural waitFor );

		virtual bool onWaitSuccess( natural waitFor );

	protected:
		String name;
		DWORD openMode;
		natural maxInstances;
		natural streamDefTimeout;
		HANDLE hPrepared;
		HANDLE fakeEvent;
		virtual natural doWait(natural waitFor, natural timeout) const ;
	};
	
}