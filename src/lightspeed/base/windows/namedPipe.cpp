#include "winpch.h"
#include "namedPipe.h"
#include "..\exceptions\systemException.h"
#include "..\..\mt\exceptions\timeoutException.h"
#include "..\exceptions\fileExceptions.h"
#include "..\countof.h"
#include "../containers/autoArray.tcc"

namespace LightSpeed{



	bool NamedPipeServer::hasItems() const
	{
		return hPrepared != 0;
	}

	PNetworkStream NamedPipeServer::getNext()
	{
		if (storedExcept) {
			storedExcept->throwAgain(THISLOCATION);
		}
		if (hPrepared == 0) {
			preparePipe();
		}
		DWORD wrt;
		if (WaitForSingleObject(hEvent,(DWORD)connectTimeout) == WAIT_TIMEOUT) throw TimeoutException(THISLOCATION);
		BOOL bres = GetOverlappedResult(hPrepared,&ovr,&wrt,TRUE);
		if (bres == FALSE) {
			throw ErrNoException(THISLOCATION,GetLastError());
		}
		bool allowRead = (dwOpenMode &  PIPE_ACCESS_INBOUND) != 0;
		bool allowWrite = (dwOpenMode &  PIPE_ACCESS_OUTBOUND) != 0;
		PNetworkStream res = new NamedPipe(lpName,hPrepared,allowRead,allowWrite);
		res->setTimeout(streamTimeout);
		hPrepared = 0;
		CloseHandle(hEvent);
		hEvent = 0;
		try {
			preparePipe();
		} catch (Exception &e) {
			storedExcept = e.clone();
		} 

		return res;
	}

	PNetworkAddress NamedPipeServer::getPeerAddr() const
	{
		return new NamedPipeServerAddr(lpName);
	}

	PNetworkAddress NamedPipeServer::getLocalAddr() const
	{
		return new NamedPipeServerAddr(lpName);
	}

	NamedPipeServer::NamedPipeServer( ConstStrW lpName, DWORD dwOpenMode, DWORD dwPipeMode, 
		DWORD nMaxInstances, DWORD nOutBufferSize, DWORD nInBufferSize,
		natural connectTimeout,
		natural streamTimeout,
		ISecurityAttributes *secAttrs /*= 0*/ )

		:lpName(lpName),dwOpenMode(dwOpenMode),dwPipeMode(dwPipeMode),nMaxInstances(nMaxInstances)
		,nOutBufferSize(nOutBufferSize),nInBufferSize(nInBufferSize)
		,secAttrs(secAttrs),hPrepared(0),hEvent(0)
		,connectTimeout(connectTimeout),streamTimeout(streamTimeout)
	{
		preparePipe();
	}

	NamedPipeServer::~NamedPipeServer()
	{
		CancelIo(hPrepared);
		CloseHandle(hEvent);
		CloseHandle(hPrepared);
	}

	void NamedPipeServer::preparePipe()
	{
		if (hPrepared) return;
		hEvent = CreateEvent(0,TRUE,0,0);
		if (hEvent == 0 || hEvent == INVALID_HANDLE_VALUE) {
			throw ErrNoException(THISLOCATION,GetLastError());
		}
		HANDLE h = CreateNamedPipeW(lpName.c_str()
			,dwOpenMode | FILE_FLAG_OVERLAPPED,dwPipeMode,nMaxInstances,nOutBufferSize
			,nInBufferSize,0,secAttrs?secAttrs->getSecurityAttributes():0);
		if (h == 0 || h == INVALID_HANDLE_VALUE) {
			DWORD err = GetLastError();
			CloseHandle(hEvent); hEvent = 0;
			throw ErrNoException(THISLOCATION,err);
		}
		ZeroMemory(&ovr,sizeof(ovr));
		ovr.hEvent = hEvent;
		BOOL b = ConnectNamedPipe(h,&ovr);
		if (b == TRUE) {
			SetEvent(hEvent);
		} else {
			DWORD err = GetLastError();
			if (err == ERROR_PIPE_CONNECTED) SetEvent(hEvent);
			else if (err != ERROR_IO_PENDING) {
				CloseHandle(hEvent); 
				hEvent = 0;
				CloseHandle(h);
				throw ErrNoException(THISLOCATION,GetLastError());
			}
		}
		hPrepared = h;
	}

	HANDLE NamedPipeServer::getWaitableObject(natural waitFor) const
	{
		 return hEvent;
	}

	natural NamedPipeServer::getDefaultWait() const
	{
		return waitForInput;
	}


	void NamedPipeServer::setTimeout( natural time_in_ms )
	{
		connectTimeout = time_in_ms;
	}

	natural NamedPipeServer::getTimeout() const
	{
		return connectTimeout;
	}

	natural NamedPipeServer::doWait( natural waitFor, natural timeout ) const
	{
		if (waitFor & waitForInput) {
			return WaitForSingleObject(hEvent,(DWORD)timeout) == WAIT_TIMEOUT?0:waitForInput;
		} else  {
			Sleep((DWORD)timeout);
			return 0;
		}
			
	}

	HANDLE NamedPipeServer::getWaitHandle( natural waitFor )
	{
		if (hEvent == 0) preparePipe();
		return hEvent;	
	}

	bool NamedPipeServer::onWaitSuccess( natural waitFor )
	{
		return true;		
	}


	natural NamedPipe::read( void *buffer, natural size )
	{
		if (size == 0 || eof) return 0;
		natural res = wait(waitForInput,defTimeout);
		if (res == 0) throw TimeoutException(THISLOCATION);
		if (!eof)
			{
			Synchronized<FastLock> _(lk);
			if (size < rdBuffUse) {
				memcpy(buffer,readBuff,size);
				memcpy(readBuff,readBuff,rdBuffUse - size);
				rdBuffUse-=size;
				return size;
			} else {
				memcpy(buffer,readBuff,rdBuffUse);
				size = rdBuffUse;
				rdBuffUse = 0;
				SyncReleased<FastLock> _(lk);
				internalRead();
				return size;
			}
		
		} else {
			return 0;
		}
	}

	natural NamedPipe::write( const void *buffer, natural size )
	{
		if (size == 0) return 0;
		natural res = wait(waitForOutput,defTimeout);
		if (res == 0) throw TimeoutException(THISLOCATION);

		if (size > countof(writeBuff)) size = countof(writeBuff);
		memcpy(writeBuff,buffer,size);
		wrBuffUse = size;
		internalWrite();
		return size;
	}

	
	natural NamedPipe::doWait( natural waitFor, natural timeout ) const
	{
		return const_cast<NamedPipe *>(this)->internalWait(waitFor,timeout);
	}
	natural NamedPipe::internalWait(natural waitFor, natural timeout ) 
	{
		//calculate total timeout
		Timeout tm(timeout);
		natural res = 0;
		do {
			//array for handles
			AutoArray<HANDLE, StaticAlloc<4> > harr;
			AutoArray<natural, StaticAlloc<4> > resarr;
			//request to wait for input
			if (waitFor & waitForInput) {
				//if there are unread data in the buffer, no wait needed, we just ready
				if (rdBuffUse != 0 || eof) return waitForInput;
				//if read is not charged, charge it now
				if (!readCharged) internalRead();
				//if read is still not charged, there are probably some data, we ready
				if (!readCharged) return waitForInput;
				//add wait handle
				harr.add(hReadWait);
				//mark handle op
				resarr.add(waitForInput);
			}

			//request to wait for output
			if (waitFor & waitForOutput) {
				//if write is not charget we are ready for output
				if (!writeCharged) return waitForOutput;
				//otherwise add handle
				harr.add(hWriteWait);
				//mark handle op
				resarr.add(waitForOutput);
			}

			//no handles, this emulates any other unsupported event
			if (harr.empty()) {
				//because the event never happen, just sleep for given period				
				Sleep((DWORD)tm.getRemain().msecs());
				//and return zero
				return 0;
			}

			//perform waiting
			DWORD wres = WaitForMultipleObjects((DWORD)harr.length(),harr.data(),FALSE,(DWORD)tm.getRemain().msecs());
			//in case of timeout
			if (wres == WAIT_TIMEOUT) return waitTimeout;
			//receive signaled handle
			DWORD idx = wres - WAIT_OBJECT_0;
			
			//if operation is waiting for input
			if (resarr[idx] == waitForInput) {
				//finish read
				if (finishRead()) res |= waitForInput;
			//if operation si waiting for output
			} else if (resarr[idx] == waitForOutput) {
				//finish write and mark output if finished
				if (finishWrite()) res |= waitForOutput;
			}
			//repeat when we need to wait
		} while (res == 0);
		return res;
	}


			
	natural NamedPipe::peek( void *buffer, natural size ) const
	{
		if (size == 0 || eof) return 0;
		if (rdBuffUse == 0) {
			if (wait(waitForInput,defTimeout) == 0) throw TimeoutException(THISLOCATION);
		}
		if (eof) return 0;
		if (size < rdBuffUse) size = rdBuffUse;
		memcpy(buffer,readBuff,size);
		return size;
	}

	bool NamedPipe::canRead() const
	{
		return !eof;
	}

	bool NamedPipe::canWrite() const
	{
		return true;
	}

	void NamedPipe::flush()
	{
		if (writeCharged) {
			bool rep;
			Timeout tm(defTimeout);
			do {
				if (WaitForSingleObject(hWriteWait,(DWORD)tm.getRemain().msecs()) == WAIT_TIMEOUT)
					throw TimeoutException(THISLOCATION);
				rep = !finishWrite();
			} while (rep);
		}
	}

	natural NamedPipe::dataReady() const
	{
		if (eof) return 1;
		if (rdBuffUse) return rdBuffUse;
		return 0;
	}

	void NamedPipe::closeOutput()
	{
	}


	natural NamedPipe::getDefaultWait() const
	{
		return waitForInput;
	}

	NamedPipe::NamedPipe(String name, HANDLE hPipe, bool allowRead, bool allowWrite )
		:hPipe(hPipe),hReadWait(0),hWriteWait(0),rdBuffUse(0),wrBuffUse(0),readCharged(false),writeCharged(false)
		,name(name),eof(!allowRead),allowWrite(allowWrite)
	{
		if (allowRead) {
			hReadWait = CreateEvent(0,TRUE,FALSE,0);
			if (hReadWait == NULL) 
				throw ErrNoException(THISLOCATION,GetLastError());
		}
		if (allowWrite) {
			hWriteWait = CreateEvent(0,TRUE,FALSE,0);
			if (hWriteWait == NULL) {
				CloseHandle(hReadWait);
				throw ErrNoException(THISLOCATION,GetLastError());
			}		
		}
		ZeroMemory(&ovrRead,sizeof(ovrRead));
		ZeroMemory(&ovrWrite,sizeof(ovrWrite));
		ovrRead.hEvent = hReadWait;
		ovrWrite.hEvent = hWriteWait;
		if (!eof) internalRead();

	}

	NamedPipe::~NamedPipe()
	{

		if (writeCharged) {
			try {
				flush();
			} catch (...) {

			}
		}
		if (readCharged) CancelIo(hPipe);
		CloseHandle(hReadWait);
		CloseHandle(hWriteWait);
		CloseHandle(hPipe);
	}

	
	void NamedPipe::internalRead()
	{
		Synchronized<FastLock> _(lk);
		//read is not charged or there are unread data
		if (readCharged || rdBuffUse || eof) return;		
		ZeroMemory(&ovrRead,sizeof(ovrRead));
		ovrRead.hEvent = hReadWait;
		ResetEvent(hReadWait);
		DWORD wrt;
		BOOL res = ReadFile(hPipe,readBuff,(DWORD)countof(readBuff),&wrt,&ovrRead);
		if (res == FALSE) {
			DWORD err = GetLastError();
			if (err != ERROR_IO_PENDING) {
				if (err == ERROR_BROKEN_PIPE) {
					eof = true;
					readCharged = false;
					return;
				} else {
					throw FileIOError(THISLOCATION,err,name);
				}
			}
			readCharged = true;
		} else {
			if (wrt == 0) {
				eof = true;
			} else {
				rdBuffUse = wrt;
				SetEvent(hReadWait);
				readCharged = false;
			}
		}
	}

	bool NamedPipe::finishRead() {
		Synchronized<FastLock> _(lk);
		//read is not charged or there are unread data
		if (!readCharged || rdBuffUse || eof) return true;
		readCharged = false;
		DWORD wrt;
		BOOL res = GetOverlappedResult(hPipe,&ovrRead,&wrt,FALSE);
		if (res == FALSE) {
			DWORD err = GetLastError();
			if (err == ERROR_HANDLE_EOF || err == ERROR_BROKEN_PIPE) {
				eof = true;
			} else if (err == ERROR_OPERATION_ABORTED) {
				return false;			
			} else {
				throw FileIOError(THISLOCATION,err,name);
			}
		}
		rdBuffUse = wrt;
		return true;
	}
	

	void NamedPipe::internalWrite() 
	{
		Synchronized<FastLock> _(lk);
		if (writeCharged || !wrBuffUse) return;		
		do {
			ZeroMemory(&ovrWrite,sizeof(ovrWrite));
			ovrWrite.hEvent = hWriteWait;
			ResetEvent(hWriteWait);
			DWORD wrt = 0;
			BOOL res = WriteFile(hPipe,writeBuff,(DWORD)wrBuffUse,&wrt,&ovrWrite);
			if (res == FALSE || wrt == 0) {
				DWORD err = GetLastError();
				if (err != ERROR_IO_PENDING) throw FileIOError(THISLOCATION,err,name);
				writeCharged = true;
				break;
			} else {
				writeCharged = false;
				if (wrt == wrBuffUse) {
					wrBuffUse = 0;
					break;
				} else {
					memmove(writeBuff,writeBuff+wrt,wrBuffUse-wrt);
				}
			}
		}
		while (true);
	}

	bool NamedPipe::finishWrite() {
		Synchronized<FastLock> _(lk);
		//write is not charged or there are no more data
		if (!writeCharged || !wrBuffUse ) return true;
		writeCharged = false;
		DWORD wrt;
		BOOL res = GetOverlappedResult(hPipe,&ovrWrite,&wrt,FALSE);
		if (res == FALSE) {
			DWORD err = GetLastError();
			if (err == ERROR_OPERATION_ABORTED)
				return false;
			throw FileIOError(THISLOCATION,err,name);
		}
		if (wrt == wrBuffUse) {
			wrBuffUse = 0;
			return true;
		} else {
			if (wrt > wrBuffUse) throw FileIOError(THISLOCATION, -1, "Internal error - undefined return value of GetOverlappedResult");
			memmove(writeBuff,writeBuff+wrt,wrBuffUse - wrt);
			internalWrite();
			return !writeCharged;
		}

	}

	HANDLE NamedPipe::getWaitHandle( natural waitFor )
	{
		if (waitFor == waitForInput) {
			//if there are unread data in the buffer, no wait needed, we just ready
			if (rdBuffUse != 0 || eof) {
				SetEvent(hReadWait);
			} else {
				//if read is not charged, charge it now
				if (!readCharged) internalRead();
				//if read is still not charged, there are probably some data, we ready
				if (!readCharged) {
					SetEvent(hReadWait);
				}
			}
			return hReadWait;
		} else if (waitFor == waitForOutput) {
			//if write is not charget we are ready for output
			if (!writeCharged) SetEvent(hWriteWait);

			return hWriteWait;

		} else throw InvalidParamException(THISLOCATION,1,"Invalid operation");
	}

	bool NamedPipe::onWaitSuccess( natural waitFor )
	{
		//if operation is waiting for input
		if (waitFor == waitForInput) {
			//finish read
			return finishRead();
			//if operation si waiting for output
		} else if (waitFor == waitForOutput) {
			//finish write and mark output if finished
			return finishWrite();
		} else throw InvalidParamException(THISLOCATION,1,"Invalid operation");
	}


	NamedPipeServerAddr::NamedPipeServerAddr( String pipeName )
	{
		this->pipeName = pipeName;
	}

	natural NamedPipeServerAddr::getSockAddress( void *buffer, natural size ) const
	{
		return 0;
	}

	bool NamedPipeServerAddr::equalTo( const INetworkAddress &other ) const
	{
		const NamedPipeServerAddr *o = dynamic_cast<const NamedPipeServerAddr *>(&other);
		if (o == 0) return false;
		return o->pipeName == pipeName;
	}

	bool NamedPipeServerAddr::lessThan( const INetworkAddress &other ) const
	{
		const NamedPipeServerAddr *o = dynamic_cast<const NamedPipeServerAddr *>(&other);
		if (o == 0) return o < this;
		return o->pipeName < pipeName;

	}

	StringA NamedPipeServerAddr::asString( bool resolve /*= false*/ )
	{
		return pipeName.getUtf8();
	}



	NamedPipeClient::NamedPipeClient( String name, DWORD openMode, natural maxInstances, natural streamDefTimeout )
		:name(name),openMode(openMode),maxInstances(maxInstances),streamDefTimeout(streamDefTimeout)
		,hPrepared(0),fakeEvent(0)
	{
	}

	bool NamedPipeClient::hasItems() const
	{
		return maxInstances>0;
	}

	PNetworkStream NamedPipeClient::getNext()
	{
		prepare();
		PNetworkStream stream = new NamedPipe(name,hPrepared,(openMode & GENERIC_READ)!=0,(openMode & GENERIC_WRITE) !=0);
		stream->setTimeout(streamDefTimeout);
		maxInstances--;
		hPrepared = 0;
		return stream;
	}

	PNetworkAddress NamedPipeClient::getPeerAddr() const
	{
		return new NamedPipeServerAddr(name);
	}

	PNetworkAddress NamedPipeClient::getLocalAddr() const
	{
		return new NamedPipeServerAddr(name);
	}

	natural NamedPipeClient::getDefaultWait() const
	{
		return waitForOutput;
	}

	void NamedPipeClient::prepare()
	{
		if (hPrepared) return;
		if (maxInstances == 0) throwIteratorNoMoreItems(THISLOCATION,typeid(*this));
		HANDLE h = CreateFileW(name.c_str(),openMode,0,0,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,0);
		if (h == INVALID_HANDLE_VALUE || h == 0)
			throw ErrNoException(THISLOCATION,GetLastError());
		hPrepared = h;

	}

	NamedPipeClient::~NamedPipeClient()
	{
		if (hPrepared !=0) CloseHandle(hPrepared);
		if (fakeEvent != 0) CloseHandle(fakeEvent);
	}

	HANDLE NamedPipeClient::getWaitHandle( natural waitFor )
	{		
		if (fakeEvent == 0) fakeEvent = CreateEvent(0,1,0,0);
		if (hPrepared) {
			SetEvent(fakeEvent);
		} else {
			try {
					prepare();
					SetEvent(fakeEvent);
			} catch(...) {
				ResetEvent(fakeEvent);
			}
		}
		return fakeEvent;
	}

	bool NamedPipeClient::onWaitSuccess( natural waitFor )
	{
		return true;
	}

	natural NamedPipeClient::doWait( natural waitFor, natural timeout ) const
	{
		HANDLE h = const_cast<NamedPipeClient *>(this)->getWaitHandle(waitFor);
		if (WaitForSingleObject(h,(DWORD)timeout) == WAIT_TIMEOUT) return 0;
		else return waitForInput;
	}

}