#include "winpch.h"
#include <algorithm>
#include <tchar.h>
#include <AccCtrl.h>
#include <Aclapi.h>
#include <sddl.h>
#include <time.h>
#include <TlHelp32.h>
#include "../debug/dbglog.h"
#include "../exceptions/errorMessageException.h"
#include "../text/textFormat.tcc"
#include "../memory/staticAlloc.h"
#include "../framework/proginstance.h"
#include "../containers/string.tcc"
#include "../text/textstream.tcc"
#include "../text/textLineReader.h"
#include "../streams/fileiobuff.tcc"
#include <psapi.h>

namespace {

class SecurityAttributes_t: public SECURITY_ATTRIBUTES ,
						  public LightSpeed::SharedResource{
public:
	SecurityAttributes_t(bool inheritHandle) {
		this->nLength = sizeof(SECURITY_ATTRIBUTES);
		this->bInheritHandle = inheritHandle?TRUE:FALSE;
		this->lpSecurityDescriptor = 0;
	}


	SecurityAttributes_t(PSECURITY_DESCRIPTOR desc, bool inheritHandle) {
		this->nLength = sizeof(SECURITY_ATTRIBUTES);
		this->bInheritHandle = inheritHandle?TRUE:FALSE;
		this->lpSecurityDescriptor = desc;
	}
	
	~SecurityAttributes_t() {
		if (!isShared() && lpSecurityDescriptor) {
			freeSecurityDesc(lpSecurityDescriptor);
		}
	}
	

	static SecurityAttributes_t getLowIntegrity(bool inheritsHandle = false);

	static PSECURITY_DESCRIPTOR createSecurityDesc(const _TCHAR *str);
	static void freeSecurityDesc(PSECURITY_DESCRIPTOR desc);

protected:



};

PSECURITY_DESCRIPTOR SecurityAttributes_t::createSecurityDesc(const _TCHAR *str) {
	
	PSECURITY_DESCRIPTOR pSD = NULL;  
	if (ConvertStringSecurityDescriptorToSecurityDescriptor(
		str, SDDL_REVISION_1, &pSD, NULL)) 
	{
		return pSD;
	} else {
		throw LightSpeed::ErrNoException(THISLOCATION,GetLastError());
	}
}

void  SecurityAttributes_t::freeSecurityDesc(PSECURITY_DESCRIPTOR desc)
{
	LocalFree((HLOCAL)desc);
}


SecurityAttributes_t SecurityAttributes_t::getLowIntegrity(
	bool inheritsHandle) 
{


	try {
		return SecurityAttributes_t(
			createSecurityDesc(_T("S:(ML;;NW;;;LW)")),inheritsHandle);
	} catch (LightSpeed::ErrNoException &) {
		return SecurityAttributes_t(inheritsHandle);
	}

}

}

namespace LightSpeed {
struct RequestBufferHdr {
	DWORD reqSize;
	DWORD replySize;
	HANDLE hEvent;
	char data[1];
};





struct ProgInstanceBuffer {
	HWND hMsgWnd;
	HANDLE hMutex;
	HANDLE hProcess;
	RequestBufferHdr *request;
	HANDLE hRequestEvent;
	bool indaemon;
	
	ProgInstanceBuffer():hMutex(0),hMsgWnd(0),hProcess(0),request(0),hRequestEvent(0),indaemon(false) {}
	~ProgInstanceBuffer() {
		if (hMsgWnd) DestroyWindow(hMsgWnd);
		if (hMutex) CloseHandle(hMutex);
		if (hProcess) CloseHandle(hProcess);
		if (request) UnmapViewOfFile(request);
	}
};


ProgInstance::ProgInstance( const String &name )
	:name(name), buffer(0)
{
	initializeStartTime();
}

ProgInstance::ProgInstance( const ProgInstance &other )
	:name(other.name), buffer(0)
{
	initializeStartTime();
}
ProgInstance::~ProgInstance()
{
	if (buffer) delete buffer;
}

static String buildName( ConstStrW name, ConstStrW restype) 
{
	return ConstStrW(L"LightSpeed-ServiceApp-")+restype+name;
}

static ConstStrW runMutex(L"runmutex");
static ConstStrW window(L"window");


static DWORD findRunningInstance(ConstStrW name, HWND *hWndOut = 0) {
	String winname = buildName(name,window);
	HWND hWnd = FindWindowExW(HWND_MESSAGE,0,L"STATIC",winname.cStr());
	if (hWnd == 0) return 0;
		

	DWORD pid;
	if (GetWindowThreadProcessId(hWnd,&pid) == 0)
		throw ErrNoException(THISLOCATION,GetLastError());
	if (hWndOut) *hWndOut = hWnd;
	return pid;
}

typedef BOOL (WINAPI *type_ChangeWindowMessageFilter)(__in UINT message,__in DWORD dwFlag);




void ProgInstance::create()
{
	if (buffer != 0) {
		delete buffer;
		buffer = 0;
	}

	String mxname = buildName(name,runMutex);
	HANDLE h = CreateMutexW(0,TRUE,mxname.cStr());
	if (GetLastError() != 0) {
		if (h) CloseHandle(h);
		throw AlreadyRunningException(THISLOCATION,findRunningInstance(name));
	}

	String winname = buildName(name,window);

	HWND hWnd = CreateWindowW(L"STATIC",winname.cStr(),0,0,0,0,0,
		HWND_MESSAGE,0,GetModuleHandle(0),0);

	type_ChangeWindowMessageFilter changeWindowMessageFilter = 0;
	
	HMODULE hMod = GetModuleHandle(_T("User32.dll"));
	if (hMod) changeWindowMessageFilter = (type_ChangeWindowMessageFilter)GetProcAddress(hMod,"ChangeWindowMessageFilter");	
	if (changeWindowMessageFilter)
		changeWindowMessageFilter(WM_APP,MSGFLT_ADD);

	buffer = new ProgInstanceBuffer;
	buffer->hMutex = h;
	buffer->hMsgWnd = hWnd;
	DuplicateHandle(GetCurrentProcess(),GetCurrentProcess(),
		GetCurrentProcess(),&buffer->hProcess,0,FALSE,
		DUPLICATE_SAME_ACCESS);	
}

void ProgInstance::create( natural requestMaxSize )
{
	create();
}

void ProgInstance::open()
{
	if (buffer != 0) {
		delete buffer;
		buffer = 0;
	}
	HWND hWnd;
	DWORD pid = findRunningInstance(name,&hWnd);
	if (pid == 0) throw ProgInstance::NotRunningException(THISLOCATION);
	HANDLE hProcess = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE,FALSE,pid);
	if (hProcess == 0)
		throw ErrNoException(THISLOCATION,GetLastError());

	buffer = new ProgInstanceBuffer;
	buffer->hMutex = 0;
	buffer->hMsgWnd = hWnd;
	buffer->hProcess = hProcess;

}


bool ProgInstance::check() const
{
	DWORD pid = findRunningInstance(name);
	if (pid == 0) {
		return true;
	} else {
		HANDLE hProcess = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE,FALSE,pid);
		if (hProcess == 0) return false;
		CloseHandle(hProcess);
		return true;
	}
}

natural ProgInstance::request( const void *req, natural reqsize, void *reply, natural replySize, natural timeout )
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	SecurityAttributes_t lowattrs = SecurityAttributes_t::getLowIntegrity();

	DWORD pid;
	GetWindowThreadProcessId(buffer->hMsgWnd,&pid);
	HANDLE hProc = OpenProcess(PROCESS_DUP_HANDLE|SYNCHRONIZE,FALSE,pid);

	DWORD maxsize = (DWORD)(std::max(reqsize,replySize) + sizeof(RequestBufferHdr));
	HANDLE hShared = CreateFileMapping(0,&lowattrs,PAGE_READWRITE,0,maxsize,0);
	if (hShared == 0 || hShared == INVALID_HANDLE_VALUE)
		throw ErrNoException(THISLOCATION,GetLastError());
	void *buff = MapViewOfFile(hShared,FILE_MAP_ALL_ACCESS,0,0,0);
	if (buff == 0) {
		DWORD err = GetLastError();
		CloseHandle(hShared);			
		throw ErrNoException(THISLOCATION,err);
	}

	RequestBufferHdr *rq = reinterpret_cast<RequestBufferHdr *>(buff);
	rq->replySize = (DWORD)replySize;
	rq->reqSize =(DWORD)reqsize;
	HANDLE hEvent = CreateEvent(&lowattrs,0,0,0);
	memcpy(rq->data,req,reqsize);
	HANDLE hOutShared;
	if (hProc) {
		DuplicateHandle(GetCurrentProcess(),hEvent,hProc,&rq->hEvent,0,
			FALSE,DUPLICATE_SAME_ACCESS);
		DuplicateHandle(GetCurrentProcess(),hShared,hProc,&hOutShared,0,
			FALSE,DUPLICATE_SAME_ACCESS);
	} else {
		rq->hEvent = hEvent;
		hOutShared = hShared;
	}

	PostMessageW(buffer->hMsgWnd,WM_APP,	(WPARAM)hOutShared,hProc?0:GetCurrentProcessId());
	DWORD res = WaitForSingleObject(hEvent,(DWORD)timeout);
	bool issmall = false;
	if (res != WAIT_TIMEOUT) {
		if (rq->replySize <= replySize) {
			memcpy(reply,rq->data,rq->replySize);			
			replySize = rq->replySize;
		} else {
			issmall = true;
		}
	}
	UnmapViewOfFile(buff);
	CloseHandle(hShared);
	CloseHandle(hEvent);
	CloseHandle(hProc);
	if (res == WAIT_TIMEOUT)
		throw TimeoutException(THISLOCATION);
	if (issmall)
		throw ReplyTooLarge(THISLOCATION);
	return replySize;
}

natural ProgInstance::getRequestMaxSize()
{
	return 128*1024;
}

natural ProgInstance::getReplyMaxSize()
{
	if (buffer && buffer->request) return buffer->request->replySize;
	return getRequestMaxSize();
}

bool ProgInstance::anyRequest()
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	MSG msg;
	return PeekMessageW(&msg,buffer->hMsgWnd,0,0,PM_NOREMOVE) != FALSE;
}

static bool processMsg(MSG &msg,ProgInstanceBuffer *buffer) {
	if (msg.message == WM_APP) {
		WPARAM pid = msg.lParam;
		HANDLE hShared = (HANDLE)msg.wParam;
		HANDLE hProc = 0;


		if (pid != 0) {
			hProc = OpenProcess(PROCESS_DUP_HANDLE,FALSE,(DWORD)pid);
			if (hProc == 0) return false;
			if (!DuplicateHandle(hProc,hShared,GetCurrentProcess(),&hShared,0,FALSE,DUPLICATE_SAME_ACCESS))
				return false;
		} 


		buffer->request = (RequestBufferHdr *)MapViewOfFile(hShared,
			FILE_MAP_ALL_ACCESS,0,0,0);

		if (buffer->request == 0) {
			CloseHandle(hShared);
			return false;
		}

		if (hProc) {
			if (!DuplicateHandle(hProc,buffer->request->hEvent,
				GetCurrentProcess(),&buffer->hRequestEvent,0,FALSE,DUPLICATE_SAME_ACCESS)) {
					CloseHandle(hShared);
					return false;
			}
		} else {
			buffer->hRequestEvent = buffer->request->hEvent;
		}

		CloseHandle(hShared);
		if (hProc != 0) CloseHandle(hProc);
		return true;
	} 
	return false;
}

void * ProgInstance::acceptRequest()
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	if (buffer->request) return buffer->request->data;
	MSG msg;
	while (PeekMessageW(&msg,buffer->hMsgWnd,0,0,PM_REMOVE) == TRUE) {
		if (processMsg(msg,buffer))
			return buffer->request->data;
	}
	return 0;
}

void * ProgInstance::waitForRequest( natural timeout )
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	if (buffer->request) return buffer->request->data;
	UINT_PTR id = timeout != naturalNull?SetTimer(buffer->hMsgWnd,1,(DWORD)timeout,0):0;
	MSG msg;
	while (GetMessage(&msg,0,0,0)) {
		if (msg.hwnd != buffer->hMsgWnd) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			if (msg.message == WM_TIMER) {
				KillTimer(buffer->hMsgWnd,id);
				return 0;
			}
			if (msg.message == WM_APP+1) {
				return 0;
			}
			if (processMsg(msg,buffer)) {
				return buffer->request->data;
			}
		}
	}
	throw TimeoutException(THISLOCATION);
}

void * ProgInstance::getReplyBuffer()
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	if (buffer->request) return buffer->request->data;
	return 0;
}

void ProgInstance::sendReply( natural sz )
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	if (buffer->request) {
		buffer->request->replySize = (DWORD)sz;
		HANDLE hEv = buffer->hRequestEvent;
		if (hEv != 0) {
			SetEvent(hEv);
			CloseHandle(hEv);
		}
		UnmapViewOfFile(buffer->request);
		buffer->request = 0;
	}
}

void ProgInstance::sendReply( const void *data, natural sz )
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	if (buffer->request) {
		if (buffer->request->replySize < sz)		
			throw ReplyTooLarge(THISLOCATION);
		memcpy(buffer->request->data,data,sz);
		sendReply(sz);
	}
}

bool ProgInstance::imOwner() const
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	DWORD pid;
	DWORD tid = GetWindowThreadProcessId(buffer->hMsgWnd,&pid);
	return tid == GetCurrentThreadId();
}

bool ProgInstance::takeOwnership()
{
	return true;
}



void ProgInstance::waitForTerminate( natural timeout )
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	if (WaitForSingleObject(buffer->hProcess,(DWORD)timeout) == WAIT_TIMEOUT) {
		throw TimeoutException(THISLOCATION);
	}

}

void ProgInstance::cancelWaitForRequest()
{
	PostMessageW(buffer->hMsgWnd,WM_APP+1,0,0);
}
void ProgInstance::TimeoutException::message(ExceptionMsg &msg) const {
	msg("Timeout exception");
}
void ProgInstance::RequestTooLarge::message(ExceptionMsg &msg) const {
	msg("Request too large");
}
void ProgInstance::ReplyTooLarge::message(ExceptionMsg &msg) const {
	msg("Reply too large");
}
void ProgInstance::AlreadyRunningException::message(ExceptionMsg &msg) const {
	msg("Process is already running under pid: %1") << pid;
}
void ProgInstance::NotRunningException::message(ExceptionMsg &msg) const {
	msg("Process is not running");
}

void ProgInstance::NotInitialized::message(ExceptionMsg &msg) const {
	msg("Not initialized");
}

bool ProgInstance::inDaemonMode() const {
	if (!imOwner())  throw ErrorMessageException(THISLOCATION, "Access denied");
	return buffer->indaemon;
}

static HANDLE openDaemonEvent() {
	TextFormatBuff<wchar_t, StaticAlloc<256> > fmt;
	fmt("LightSpeed_DaemonMode_Event_%1%%2") << natural(GetCurrentProcessId()) << '\0';
	ConstStrW eventName = fmt.write();
	HANDLE hDemonEvent = OpenEventW(EVENT_ALL_ACCESS,FALSE,eventName.data());
	return hDemonEvent;
}

static HANDLE createDaemonEvent(DWORD pid) {
	TextFormatBuff<wchar_t, StaticAlloc<256> > fmt;
	fmt("LightSpeed_DaemonMode_Event_%1%%2") << natural(pid) << '\0';
	ConstStrW eventName = fmt.write();
	SecurityAttributes_t secatr = SecurityAttributes_t::getLowIntegrity(false);
	HANDLE hDemonEvent = CreateEventW(&secatr,TRUE,FALSE,eventName.data());
	return hDemonEvent;
}

static bool waitForInitDaemon(HANDLE hDaemonEvent, HANDLE hDaemonProcess) {
	HANDLE hWaits[2];
	hWaits[0] = hDaemonEvent;
	hWaits[1] = hDaemonProcess;
	DWORD sel = WaitForMultipleObjects(2,hWaits,FALSE,INFINITE);
	return sel == WAIT_OBJECT_0;
}

struct DaemonSharedArea {
	time_t startTime;
	time_t restartTime;
	natural restartCount;

	DaemonSharedArea() :startTime(0), restartTime(0), restartCount(0) {}
};

static DaemonSharedArea daemonProcessData;

static DWORD getParentPid() {
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(entry);
	DWORD mypid = GetCurrentProcessId();
	DWORD foundpid = 0;
	BOOL cont = Process32FirstW(hSnapShot, &entry);
	while (cont) {
		if (mypid == entry.th32ProcessID) {
			foundpid = entry.th32ParentProcessID;
			cont = FALSE;
		}
		else {
			entry.dwSize = sizeof(entry);
			Process32NextW(hSnapShot, &entry);
		}
	}
	CloseHandle(hSnapShot);
	return foundpid;

}

static String getDaemonAreaName(DWORD pid) {
	TextFormatBuff<wchar_t, StaticAlloc<256> > fmt;
	fmt("LightSpeed_DaemonMode_SharedArea_%1") << natural(pid);
	return fmt.write();
}

void ProgInstance::enterDaemonMode(natural restartOnErrorSec) {	
	LS_LOGOBJ(lg);
	if (!imOwner())  throw ErrorMessageException(THISLOCATION, "Access denied");
	DWORD masterPid = getParentPid();

	HANDLE hDaemonArea = OpenFileMappingW(FILE_MAP_READ, FALSE,getDaemonAreaName(masterPid).cStr());
	if (hDaemonArea == NULL || hDaemonArea == INVALID_HANDLE_VALUE) {
		//we are master
		DbgLog::setThreadName("watchdog",false);
		lg.info("Entering to daemon mode");

		HANDLE hDaemonArea = CreateFileMappingW(0, 0, PAGE_READWRITE, 0, sizeof(DaemonSharedArea), getDaemonAreaName(GetCurrentProcessId()).c_str());
		if (hDaemonArea == NULL || hDaemonArea == INVALID_HANDLE_VALUE) {
			throw ErrNoException(THISLOCATION, GetLastError());
		}
		DaemonSharedArea *dsa = (DaemonSharedArea *)MapViewOfFile(hDaemonArea, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		initializeStartTime();
		*dsa = daemonProcessData;
		DWORD exitCode = 0;

		//delete instance file - will be created in daemon
		delete buffer;
		buffer = 0;

		do {

			//enter into daemon
			STARTUPINFOW sinfo;
			PROCESS_INFORMATION pi;
			ZeroMemory(&sinfo, sizeof(sinfo));
			sinfo.cb = sizeof(sinfo);

			Pipe p;
			IFileExtractHandle &eh = p.getWriteEnd().getStream()->getIfc<IFileExtractHandle>();
			HANDLE hWrite;
			eh.getHandle(&hWrite, sizeof(hWrite));
			SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
			
			Pipe p2;
			IFileExtractHandle &eh2 = p2.getReadEnd().getStream()->getIfc<IFileExtractHandle>();
			HANDLE hRead;
			eh2.getHandle(&hRead, sizeof(eh2));
			SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);


			sinfo.dwFlags = STARTF_USESTDHANDLES;
			sinfo.hStdError = hWrite;
			sinfo.hStdOutput = hWrite;
			sinfo.hStdInput = hRead;

			LPWSTR cmdLine = GetCommandLineW();
			BOOL res = CreateProcessW(0, cmdLine, 0, 0, TRUE, CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP | CREATE_BREAKAWAY_FROM_JOB | CREATE_UNICODE_ENVIRONMENT | DETACHED_PROCESS | CREATE_SUSPENDED,
				0, 0, &sinfo, &pi);
			if (res == FALSE) throw ErrNoException(THISLOCATION, GetLastError());
			ResumeThread(pi.hThread);
			CloseHandle(pi.hThread);
			SeqFileInBuff<> input(p.getReadEnd());
			p.detach();
			p2.detach();

			TextLineReader<SeqTextInA> rline(input);
			while (rline.hasItems()) {
				ConstStrA line = rline.getNext();
				lg.info("%1") << line;
			}
			WaitForSingleObject(pi.hProcess, INFINITE);
			GetExitCodeProcess(pi.hProcess, &exitCode);
			CloseHandle(pi.hProcess);
			if (exitCode>=251) {
				lg.error("Process crashed with exit code %1. Will be restarted") << natural(exitCode);
				time_t prevRestart = dsa->restartTime;
				time(&dsa->restartTime);
				dsa->restartCount++;
				if (dsa->restartTime - dsa->startTime < restartOnErrorSec) {
					lg.error("Process crashed to soon, cannot be restarted");
					break;
				}
				time_t rs = dsa->restartTime - prevRestart;
				if (rs < restartOnErrorSec) {
					lg.error("Waiting for restart %1 seconds") << natural(restartOnErrorSec - rs);
					Sleep(DWORD((restartOnErrorSec - rs) * 1000));
				}
			}
		} while (exitCode >= 251);
		lg.info("Normal exit with status %1") << natural(exitCode);
		CloseHandle(hDaemonArea);
		UnmapViewOfFile(dsa);
		ExitProcess(exitCode);
	}
	else {
		DaemonSharedArea *dsa = (DaemonSharedArea *)MapViewOfFile(hDaemonArea, FILE_MAP_READ, 0, 0, 0);
		daemonProcessData = *dsa;
		UnmapViewOfFile(dsa);
		CloseHandle(hDaemonArea);

		DbgLog::setThreadName("main", false);
		buffer->indaemon = true;

	}



	
}

natural ProgInstance::getRestartCounts()
{
	return daemonProcessData.restartCount;
}

natural ProgInstance::getUpTime(bool resetOnRestart)
{
	time_t now;
	time(&now);
	if (resetOnRestart) return natural(now - daemonProcessData.restartTime);
	return natural(now - daemonProcessData.startTime);
}

void ProgInstance::initializeStartTime()
{
	if (daemonProcessData.startTime == 0) time(&daemonProcessData.startTime);
	if (daemonProcessData.restartTime == 0)  daemonProcessData.restartTime = daemonProcessData.startTime;
}

natural ProgInstance::getCPUTime()
{
	FILETIME creation;
	FILETIME exittime;
	FILETIME usertime;
	FILETIME kerneltime;

	GetProcessTimes(GetCurrentProcess(), &creation, &exittime, &kerneltime, &usertime);
	TimeStamp ut = TimeStamp::fromWindows(usertime.dwLowDateTime, usertime.dwHighDateTime);
	TimeStamp kt = TimeStamp::fromWindows(kerneltime.dwLowDateTime, kerneltime.dwHighDateTime);
	TimeStamp total = ut + kt;
	return total.getDay()*total.dayMillis + total.getTime();
}

static void *loadAndFindSystemFunction(const char *syslib, const char *fnName) {
	HMODULE hMod = GetModuleHandleA(syslib);
	if (hMod == 0) hMod = LoadLibraryA(syslib);
	if (hMod == 0) return 0;
	return GetProcAddress(hMod, fnName);
}

natural ProgInstance::getMemoryUsage()
{
	typedef BOOL
		(WINAPI *type_GetProcessMemoryInfo)(
		HANDLE Process,
		PPROCESS_MEMORY_COUNTERS ppsmemCounters,
		DWORD cb
		);

	static type_GetProcessMemoryInfo GetProcessMemoryInfo =
		(type_GetProcessMemoryInfo)
		loadAndFindSystemFunction("psapi.dll", "GetProcessMemoryInfo");

	if (GetProcessMemoryInfo) {

		PROCESS_MEMORY_COUNTERS_EX mc;
		GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&mc), sizeof(mc));;
		return natural(mc.PrivateUsage);
	}
	else {
		return naturalNull;
	}
}

void ProgInstance::terminate()
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	if (imOwner())  ExitProcess(0);
	if (!TerminateProcess(buffer->hProcess,255))
		throw ErrNoException(THISLOCATION,GetLastError());
}

ProgInstance::InstanceStage ProgInstance::getInstanceStage() const
{
	if (buffer == 0) return standard;
	if (!imOwner()) return controller;
	if (buffer->indaemon) return daemon;
	HANDLE hDaemonEvent = openDaemonEvent();
	CloseHandle(hDaemonEvent);
	if (hDaemonEvent == 0) return standard;
	else return predaemon;
}


void ProgInstance::restartDaemon()
{
	ExitProcess(251);
}

}