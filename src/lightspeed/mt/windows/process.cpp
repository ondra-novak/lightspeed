#include <algorithm>
#include "../../base/windows/winpch.h"
#include "../process.h"
#include "../../base/containers/autoArray.tcc"
#include "../../base/containers/string.h"
#include "../../base/memory/rtAlloc.h"
#include "../gate.h"
#include "../../base/exceptions/invalidParamException.h"
#include "../../base/windows/fileio.h"
#include "../../base/exceptions/fileExceptions.h"
#include "../../base/interface.tcc"
#include "../../base/memory/dynobject.h"
#include "../../base/memory/smallAlloc.h"
#include "../../base/text/textFormat.h"
#include "../../base/countof.h"


namespace LightSpeed {


	HANDLE extractHandle( PInputStream &file, IFileIOServices::StdFile hndType );
	HANDLE extractHandle( POutputStream &file, IFileIOServices::StdFile hndType );


	bool ProcessEnvKeyCompare::operator()(const ConstStrW &a, const ConstStrW &b) const {
		natural mm = std::min(a.length(),b.length());
		int res = _wcsnicmp(a.data(),b.data(), mm);
		if (res == 0) {
			return a.length() < b.length();
		} else {
			return res < 0;
		}
	}


/*	class PipeSharedWithProcess: public WindowsPipe {
	public:

		PipeSharedWithProcess(String pipeName, HANDLE hPipe, 
			bool output)
			:WindowsPipe(pipeName,hPipe,output),hProcess(0) {}

		virtual natural postIO( BOOL res, OVERLAPPED &ovr, DWORD rd ) {
			if (hProcess == 0) return WindowsPipe::postIO(res,ovr,rd);
			if (res == FALSE) {
				DWORD err = GetLastError();
				if (err == ERROR_IO_PENDING) {
					HANDLE hnd[2];
					hnd[0] = this->hEvent;
					hnd[1] = hProcess;
					DWORD wres = WaitForMultipleObjects(2,hnd,FALSE,INFINITE);
					if (wres == WAIT_OBJECT_0 + 1) {
						CancelIo(this->hFile);
					}
					GetOverlappedResult(hFile,&ovr,&rd,TRUE);
					return rd;
				} else
					throw FileIOError(THISLOCATION,err,fileName);
			} else {
				return rd;
			}						
		}

		void setProcessHandle(HANDLE hProcess) {
			if (!DuplicateHandle(GetCurrentProcess(),hProcess,
				GetCurrentProcess(),&this->hProcess,0,FALSE,
				DUPLICATE_SAME_ACCESS))
				throw ErrNoException(THISLOCATION,GetLastError());
		}

		~PipeSharedWithProcess() {
			if (hProcess) CloseHandle(hProcess);
		}

	protected:

		HANDLE hProcess;
		HANDLE hOtherSide;
	};
*/
	static void createIpcPipe(PInputStream &readEnd, POutputStream &writeEnd) {
		PInputStream tmprd;
		POutputStream tmpwr;
		IFileIOServices &svc = IFileIOServices::getIOServices();

		svc.createPipe(tmprd,tmpwr);
		readEnd = tmprd;
		writeEnd = tmpwr;
	}

	typedef SmallAlloc<256> CmdLineAlloc;
	typedef AutoArrayStream<wchar_t,CmdLineAlloc > CmdLine;

	class ProcessContext: public IProcessContext, public DynObject {
	public:

		HANDLE hProcess;
		DWORD processId;

		PInputStream pInput;
		POutputStream pOutput,pError;
		AutoArrayStream<POutputStream> extraWritePipes;
		AutoArrayStream<PInputStream> extraReadPipes;
		CmdLine cmdLine;
		CmdLine envStr;
		bool noArgs;
		DWORD extraFlags;
//		IRuntimeAlloc *alloc;



		ProcessContext(IRuntimeAlloc *alloc):
			hProcess(0),cmdLine(CmdLineAlloc(alloc)),envStr(CmdLineAlloc(alloc)),noArgs(true)
			,extraFlags(0) {}

		static ProcessContext &getCtx(IProcessContext &ctx) {
			return static_cast<ProcessContext &>(ctx);
		}
		static const ProcessContext &getCtx(const IProcessContext &ctx) {
			return static_cast<const ProcessContext &>(ctx);
		}

		~ProcessContext() {
			if (hProcess) CloseHandle(hProcess);
		}


		void setEnv( const ProcessEnv * env ) 
		{
			envStr.clear();
			if (env != 0) {
				for (ProcessEnv::Iterator iter = env->getFwIter();
					iter.hasItems();) {
						const ProcessEnv::Entity &e = iter.getNext();
						envStr.copy(e.key.getFwIter());
						envStr.write('=');
						envStr.copy(e.value.getFwIter());
						envStr.write('\0');
				}
			}
			envStr.write('\0');
		}

		///Adds process argument;
		void arg(ConstStrW a, bool first = false) {
			CmdLine &ln = cmdLine;
			if (first) {
				ln.clear();
				extraFlags = 0;
				noArgs = true;
			} else {
				ln.write(' ');
				noArgs = false;
			}
			if (a.find(' ') != naturalNull || a.find('"') != naturalNull) {
				ln.write('"');
				int slashcnt = 0;
				for (ConstStrW::Iterator iter = a.getFwIter();iter.hasItems();) {
					wchar_t x = iter.getNext();
					if (x == '\\') slashcnt++;
					else if (x == '"') {
						slashcnt>>=1;
						while (slashcnt) {ln.write('\\');slashcnt--;}
						ln.write(x);
					} else {
						while (slashcnt) {ln.write('\\');slashcnt--;}
						ln.write(x);
					}
				}
				ln.write('"');
			} else {
				ln.copy(a.getFwIter());
			}
		}
	};



	Process::Process() {}
	

	Process::Process(String name)
		:processName(name),context(new ProcessContext(&StdAlloc::getInstance())) {
			clear();
	}
	Process::Process(String name,IRuntimeAlloc &alloc)
		:processName(name),context(new(alloc) ProcessContext(&alloc)) {			
			clear();
	}

	Process &Process::stdOut(Process &other) {		
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ProcessContext &octx = ProcessContext::getCtx(*other.context);
		createIpcPipe(octx.pInput,ctx.pOutput);
		return *this;
	}
	Process &Process::stdErr(Process &other) {		
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ProcessContext &octx = ProcessContext::getCtx(*other.context);
		createIpcPipe(octx.pInput,ctx.pError);
		return *this;
	}

	Process & Process::stdOutErr( Process &other)
	{
		Process &p = stdOut(other);
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.pError = ctx.pOutput;
		return p;
	}

	Process &Process::stdIn(Process &other) {		
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ProcessContext &octx = ProcessContext::getCtx(*other.context);
		createIpcPipe(ctx.pInput,octx.pOutput);
		return *this;
	}


	HANDLE getFdFromStream(PInOutStream hndl, const ProgramLocation &loc) {
		IFileExtractHandle *fdinfo = dynamic_cast<IFileExtractHandle *>(hndl.get());
		if (fdinfo == 0) {
			throw InvalidParamException(loc,1,"Object must be native file, pipe or socket");
		}
		HANDLE fd;
		try {
			fdinfo->getHandle(&fd,sizeof(fd));
		} catch (Exception &e) {
			throw InvalidParamException(loc,1,"Object must be based on WINDOWS HANDLE") << e;
		}
		return fd;
	}


	Process &Process::stdIn(const SeqFileInput &input) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.pInput = input.getStream();
		return *this;
	}

	Process &Process::stdIn(SeqFileOutput &output) {

		ProcessContext &ctx = ProcessContext::getCtx(*context);				
		POutputStream local;
		createIpcPipe(ctx.pInput,local);
		output = SeqFileOutput(local);
		return *this;
	}


	Process &Process::stdOut(const SeqFileOutput &output) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);

		ctx.pOutput = output.getStream();

		return *this;
	}

	Process &Process::stdOut(SeqFileInput &input) {

		ProcessContext &ctx = ProcessContext::getCtx(*context);				
		PInputStream local;
		createIpcPipe(local,ctx.pOutput);
		input = SeqFileInput(local);
		return *this;
	}

	Process & Process::stdOutErr(SeqFileInput &input)
	{
		Process &p = stdOut(input);
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.pError = ctx.pOutput;
		return p;
	}

	Process & Process::stdOutErr(const SeqFileOutput &output)
	{
		Process &p = stdOut(output);
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.pError = ctx.pOutput;
		return p;
	}


	Process &Process::stdErr(const SeqFileOutput &output) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);

		ctx.pError = output.getStream();

		return *this;
	}

	Process &Process::stdErr(SeqFileInput &input) {

		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		PInputStream local;
		createIpcPipe(local,ctx.pError);
		input = SeqFileInput(local);
		return *this;
	}



	Process &Process::setEnv(const ProcessEnv *env) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.setEnv(env);
		return *this;
	}

		///Clears arguments
	Process &Process::clear(){
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.arg(processName,true);
		//discard all extra pipes refered by arguments
		ctx.extraWritePipes.clear();
		ctx.extraReadPipes.clear();
		return *this;
	}

	///True, if process has no arguments
	bool Process::empty() const {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		return ctx.noArgs;

	}


	///Adds process argument;
	Process &Process::arg(const String &a) {
		return arg(ConstStrW(a));
	}

	///Adds process argument;
	Process &Process::arg(ConstStrW a) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		ctx.arg(a);
		return *this;
	}

	///Adds process argument;
	Process &Process::arg(const StringA &a) {
		return arg(String(a));
	}
	
	///Adds process argument;
	Process &Process::arg(ConstStrA a) {
		return arg(String(a));
	}
	///Adds process argument;
	Process &Process::arg(const char *a) {
		return arg(String(a));
	}

	///Adds process argument;
	Process &Process::arg(const wchar_t *a) {
		return arg(String(a));

	}

	///Adds process argument;
	Process &Process::arg(natural number, natural base) {
		char buff[100];
		return arg(LightSpeed::_intr::numberToString(number,buff,100,base));
	}

	///Adds process argument;
	Process &Process::arg(integer number, natural base) {
		char buff[100];
		return arg(LightSpeed::_intr::numberToString(number,buff,100,base));

	}

	///Adds process argument;
	Process &Process::arg(float number) {
		char buff[100];
		return arg(LightSpeed::_intr::realToString(number,buff,100,10,-5,8));

	}

	///Adds process argument;
	Process &Process::arg(double number) {
		char buff[100];
		return arg(LightSpeed::_intr::realToString(number,buff,100,10,-5,8));
	}

	///Adds process extra pipe
	/**
	 * @param pipeIn variable receives stream which can
	 * be used to transfer data from the new process 
	 * to the current application.
	 */
	 
	Process &Process::arg(SeqFileInput &pipeIn) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		PInputStream readEnd;
		POutputStream writeEnd;
		createIpcPipe(readEnd,writeEnd);
		pipeIn = SeqFileInput(readEnd);
		ctx.extraWritePipes.write(writeEnd);		
		HANDLE h;
		writeEnd->getIfc<IFileExtractHandle>().getHandle(&h,sizeof(h));
		return arg(getFDName(h,false,true));		
	}

	///Adds process extra pipe
	/**
	 * @param pipeOut variable receives stream which can
	 * be used to transfer data from the current application 
	 * to the new application.
	 */
	Process &Process::arg(SeqFileOutput &pipeOut) {
		PInputStream readEnd;
		POutputStream writeEnd;
		createIpcPipe(readEnd,writeEnd);
		pipeOut = SeqFileOutput(writeEnd);
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		ctx.extraReadPipes.write(readEnd);		
		HANDLE h;
		writeEnd->getIfc<IFileExtractHandle>().getHandle(&h,sizeof(h));
		return arg(getFDName(h,false,true));		
	}

	static inline bool isScript( const String &processName ) 
	{
		ConstStrW ext = processName.tail(4);
		return ext == ConstStrW(L".bat") || ext == ConstStrW(L".cmd");
	}

	void Process::start() {

		STARTUPINFOW nfo;
		PROCESS_INFORMATION pi;
		initStruct(nfo).cb = sizeof(nfo);
		DWORD flags = CREATE_UNICODE_ENVIRONMENT|CREATE_NEW_PROCESS_GROUP;
		bool stdHandles = false;

		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		if (ctx.pInput != nil || ctx.pOutput != nil || ctx.pError != nil) {

			nfo.dwFlags |= STARTF_USESTDHANDLES;
			nfo.hStdOutput = extractHandle(ctx.pOutput, IFileIOServices::stdOutput);
			nfo.hStdInput = extractHandle(ctx.pInput, IFileIOServices::stdInput);
			nfo.hStdError = extractHandle(ctx.pError, IFileIOServices::stdError);

			//FIXME: CREATE_NO_WINDOW for console application will not redirect child's standard output to parent console
			/* That is the reason, why this flag is set for non-console parents. 
			  Child application will share parents console and there is no need to specify this flag */
			if (GetConsoleWindow() == 0)	flags |= CREATE_NO_WINDOW;
			
			stdHandles = true;

		}

		for (natural k = 0; k < ctx.extraWritePipes.length(); k++) {
			HANDLE h;
			ctx.extraWritePipes.getArray()[k]->getIfc<IFileExtractHandle>().getHandle(&h,sizeof(h));
			SetHandleInformation(h,HANDLE_FLAG_INHERIT,HANDLE_FLAG_INHERIT);
		}
		for (natural k = 0; k < ctx.extraReadPipes.length(); k++) {
			HANDLE h;
			ctx.extraReadPipes.getArray()[k]->getIfc<IFileExtractHandle>().getHandle(&h,sizeof(h));
			SetHandleInformation(h,HANDLE_FLAG_INHERIT,HANDLE_FLAG_INHERIT);
		}

		wchar_t cmdline[32768];
		if (isScript(processName)) {
			wcscpy_s(cmdline,L"cmd /S /C \"");
			wcsncat_s(cmdline,ctx.cmdLine.data(),ctx.cmdLine.length());
			wcscat_s(cmdline,L"\"");
		} else {
			wcsncpy_s(cmdline,ctx.cmdLine.data(),ctx.cmdLine.length());		
		}
		BOOL shareHandles = (stdHandles || !ctx.extraReadPipes.empty()|| !ctx.extraWritePipes.empty())?TRUE:FALSE;
		BOOL b = CreateProcessW(0,cmdline,0,0,shareHandles,flags|ctx.extraFlags,
			ctx.envStr.empty()?0:(LPVOID)ctx.envStr.data(),
			cwd.empty()?0:cwd.c_str(),&nfo,&pi);

		if (b == FALSE) {
			throw UnableToStartProcessException(THISLOCATION,GetLastError(),cmdline);
		}

		//close thread handle, we don't need it for control process
		CloseHandle(pi.hThread);

		//close other sides of redirections
		ctx.pOutput = nil;
		ctx.pInput = nil;
		ctx.pError = nil;

		//clear temporary command line
		clear();
		//if context contains handle to old process, closes it first
		if (ctx.hProcess) CloseHandle(ctx.hProcess);
		//store handle to process
		ctx.hProcess = pi.hProcess;
		//store process id
		ctx.processId = pi.dwProcessId;
		//close gate
		finishGate.close();
	}

	static BOOL _stdcall ProcessCloseAllWindows(HWND hWnd, LPARAM processId) {
		DWORD pid = 0;
		GetWindowThreadProcessId(hWnd,&pid);
		if (pid == processId) 
			PostMessage(hWnd, WM_CLOSE,0,0);
		return TRUE;
	}

	void Process::stop( bool force /*= false*/ )
	{
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		if (force) TerminateProcess(ctx.hProcess,UINT(-1));
		else {
			EnumWindows(&ProcessCloseAllWindows,ctx.processId);
			GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT , ctx.processId);
		}
	}

	integer Process::join() {
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		WaitForSingleObject(ctx.hProcess,INFINITE);
		finishGate.open();
		return getExitCode();
	}

	bool Process::join(const Timeout &tm) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		bool res = WaitForSingleObject(ctx.hProcess,DWORD(tm.getRemain().msecs())) 
				!= WAIT_TIMEOUT;
		if (res) finishGate.open();
		return res;
	}

	void Process::detach() {
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		CloseHandle(ctx.hProcess);
		ctx.hProcess = 0;
		ctx.pInput = nil;
		ctx.pOutput = ctx.pError = nil;	
	}

	Process::~Process() try 
	{
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		if (ctx.hProcess != 0) join();
	} catch (...) {
		if (std::uncaught_exception()) return;
	}

	LightSpeed::integer Process::getExitCode() const
	{
		DWORD exitCode;
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		if (!GetExitCodeProcess(ctx.hProcess,&exitCode))
			throw ErrNoException(THISLOCATION,GetLastError());
		return exitCode;
	}

	bool Process::isRunning() const
	{
		return !finishGate.isOpened();
	}

	ProcessEnv Process::getEnv()
	{
		class AutoFree {
		public:
			wchar_t *x;
			AutoFree(wchar_t *x):x(x) {}
			~AutoFree() {FreeEnvironmentStringsW(x);}
		};
	
		wchar_t *w = GetEnvironmentStringsW();
		AutoFree af(w);

	
		ConstStrW line(w);
		ProcessEnv env;
		while (!line.empty()) {
			w+=line.length()+1;
			natural x = line.find('=');
			if (x != naturalNull) {				
					env.replace(line.head(x),line.offset(x+1));
			}	
			line = w;
		}
		return env;
	}

	Process & Process::cmdLine( String cmdLine, bool append /*= false*/ )
	{
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		if (append) ctx.cmdLine.write(' '); else ctx.cmdLine.clear();
		ctx.cmdLine.copy(cmdLine.getFwIter());
		return *this;

	}

	LightSpeed::String Process::getCmdLine() const
	{
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		return ctx.cmdLine.getArray();
	}

	Process & Process::breakJob()
	{
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		ctx.extraFlags |= CREATE_BREAKAWAY_FROM_JOB;
		return *this;
	}

	Process & Process::newSession()
	{
		ProcessContext &ctx = ProcessContext::getCtx(*context);		
		ctx.extraFlags |= DETACHED_PROCESS;
		return *this;
	}
	HANDLE extractHandle( PInputStream &file, IFileIOServices::StdFile hndType )
	{
		HANDLE h;
		if (file == nil) {			
			file = IFileIOServices::getIOServices().openStdFile(hndType).get();
		}
		file->getIfc<IFileExtractHandle>().getHandle(&h,sizeof(HANDLE));
		SetHandleInformation(h,HANDLE_FLAG_INHERIT,HANDLE_FLAG_INHERIT);
		return h;
	}

	HANDLE extractHandle( POutputStream &file, IFileIOServices::StdFile hndType )
	{
		HANDLE h;
		if (file == nil) {			
			file = IFileIOServices::getIOServices().openStdFile(hndType).get();
		}
		file->getIfc<IFileExtractHandle>().getHandle(&h,sizeof(HANDLE));
		SetHandleInformation(h,HANDLE_FLAG_INHERIT,HANDLE_FLAG_INHERIT);
		return h;
	}



}