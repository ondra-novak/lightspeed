/*
 * process.cpp
 *
 *  Created on: 22.4.2011
 *      Author: ondra
 */
#include <unistd.h>
#include <unistd.h>
#include <unistd.h>
#include "../process.h"
#include "../../base/containers/autoArray.h"
#include "../../base/memory/rtAlloc.h"
#include "../../base/text/textFormat.tcc"
#include "../../base/memory/staticAlloc.h"
#include "../../base/linux/fileio.h"
#include "../../base/memory/smallAlloc.h"
#include <fcntl.h>
#include <wait.h>
#include "../../base/linux/env.h"
#include "../../base/memory/singleton.h"
#include <string.h>
#include "llmutex.h"
#include "../../base/interface.tcc"
#include "../../base/containers/string.tcc"
#include "llevent.h"
#include "../../mt/spinlock.h"
#include "../../base/containers/stringpool.tcc"
#include "../../platform.h"
#include "../../base/debug/dbglog.h"

namespace LightSpeed {

	bool ProcessEnvKeyCompare::operator ()(const ConstStrW &a, const ConstStrW &b) const {
		return a < b;
	}

	class LinuxProcessContext: public IProcessContext, public DynObject {
	public:
		static LinuxProcessContext &getCtx(IProcessContext &ctx) {
			return static_cast<LinuxProcessContext &>(ctx);
		}
		static const LinuxProcessContext &getCtx(const IProcessContext &ctx) {
			return static_cast<const LinuxProcessContext &>(ctx);
		}




		const ProcessEnv *curEnv;
		typedef RefCntPtr<IRefCntInterface> PStream;
		AutoArray<StringA, RTAlloc> cmdLine;
		IRuntimeAlloc &alloc;
		PInputStream input;
		POutputStream output,error;
		AutoArray<PStream, RTAlloc> extraStreams;
		int pid;
		integer exitCode;

		LinuxProcessContext()
			:curEnv(0),cmdLine(RTAlloc(StdAlloc::getInstance()))
			,alloc(StdAlloc::getInstance())
			,extraStreams(RTAlloc(StdAlloc::getInstance()))
			,pid(-1),exitCode(-1) {}
		LinuxProcessContext(IRuntimeAlloc &alloc)
			:curEnv(0),cmdLine(RTAlloc(alloc))
			,alloc(alloc)
			,extraStreams(RTAlloc(alloc))
			,pid(-1),exitCode(-1) {}


	};


	Process::Process() {}
	Process::Process(String processName)
		:processName(processName),context(new LinuxProcessContext()) {}
	Process::Process(String processName, IRuntimeAlloc &alloc)
		:processName(processName),context(new(alloc) LinuxProcessContext(alloc)) {}
	Process::~Process() {
		if (context) join();
	}


	Process &Process::clear() {
		LinuxProcessContext::getCtx(*context).cmdLine.clear();
		LinuxProcessContext::getCtx(*context).extraStreams.clear();
		return *this;
	}

	bool Process::empty() const {
		return LinuxProcessContext::getCtx(*context).cmdLine.empty();
	}

	Process &Process::arg(const String &a) {
		return arg(ConstStrW(a));
	}

	Process &Process::arg(ConstStrW arg) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		ctx.cmdLine.add(String::getUtf8(arg,ctx.alloc));
		return *this;
	}

	Process &Process::arg(ConstStrA aa) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		StringA a(aa,ctx.alloc);
		ctx.cmdLine.add(a);
		return *this;
	}

	Process &Process::arg(const StringA &a) {
		return arg(ConstStrA(a));
	}

	Process &Process::arg(const char *a) {
		return arg(ConstStrA(a));
	}

	Process &Process::arg(const wchar_t *a) {
		return arg(ConstStrW(a));
	}

	///Adds process argument;
	Process &Process::arg(natural number, natural base) {
		TextFormatBuff<char, StaticAlloc<256> > fmt;
		fmt.setBase(base);
		fmt("%1") << number;
		return arg(ConstStrA(fmt.write()));
	}

	///Adds process argument;
	Process &Process::arg(integer number, natural base) {
		TextFormatBuff<char, StaticAlloc<256> > fmt;
		fmt.setBase(base);
		fmt("%1") << number;
		return arg(ConstStrA(fmt.write()));
	}

	///Adds process argument;
	Process &Process::arg(float number) {
		TextFormatBuff<char, StaticAlloc<256> > fmt;
		fmt("%1") << number;
		return arg(ConstStrA(fmt.write()));

	}

	///Adds process argument;
	Process &Process::arg(double number) {
		TextFormatBuff<char, StaticAlloc<256> > fmt;
		fmt("%1") << number;
		return arg(ConstStrA(fmt.write()));
	}

	Process &Process::arg(SeqFileInput &pipeIn) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		Pipe pipe;
		pipeIn = pipe.getReadEnd();
		SeqFileOutput pipeOut = pipe.getWriteEnd();
		ctx.extraStreams.add(pipeOut.getStream().get());
		int fd;
		pipeOut.getStream()->getIfc<IFileExtractHandle>().getHandle(&fd,sizeof(fd));
		return arg(getFDName(fd,false,true,true));
	}

	Process &Process::arg(SeqFileOutput &pipeOut) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		Pipe pipe;
		pipeOut = pipe.getWriteEnd();
		SeqFileInput pipeIn = pipe.getReadEnd();
		ctx.extraStreams.add(pipeIn.getStream().get());
		int fd;
		pipeIn.getStream()->getIfc<IFileExtractHandle>().getHandle(&fd,sizeof(fd));
		return arg(getFDName(fd,true,false,true));
	}

	Process &Process::cmdLine(String cmdLine, bool append) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		if (!append) clear();
		AutoArrayStream<char, SmallAlloc<256> >buffer;
		WideToUtf8Reader<String::Iterator> iter(cmdLine.getFwIter());
		bool q = false;
		while (iter.hasItems()) {
			char c = iter.getNext();
			if (c == ' ' && !q) {
				if (!buffer.empty()) {
					arg(ConstStrA(buffer.getArray()));
					buffer.clear();
				}
			} else if (c == '"' && q) {
				arg(ConstStrA(buffer.getArray()));
				buffer.clear();
				q = false;
			} else if (c == '"' && !q) {
				if (!buffer.empty()) {
					arg(ConstStrA(buffer.getArray()));
					buffer.clear();
				}
				q = true;
			} else if (c == '\\' && iter.hasItems()) {
				buffer.write(iter.getNext());
			} else {
				buffer.write(c);
			}
		}
		if (!buffer.empty()) {
			arg(ConstStrA(buffer.getArray()));
		}

		if (!append && !ctx.cmdLine.empty()) {
			processName = ctx.cmdLine[0];
			ctx.cmdLine.erase(0);
		}

		return *this;
	}

	Process &Process::stdOut(const SeqFileOutput &stream) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		ctx.output = stream.getStream();
		return *this;
	}

	Process &Process::stdErr(const SeqFileOutput &stream) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		ctx.error = stream.getStream();
		return *this;
	}


	Process &Process::stdOutErr(const SeqFileOutput &stream) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		ctx.output = ctx.error = stream.getStream();
		return *this;
	}

	Process &Process::stdIn(const SeqFileInput &stream) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		ctx.input = stream.getStream();
		return *this;
	}


	Process &Process::stdOut(SeqFileInput &stream) {
		Pipe pipe;
		stream = pipe.getReadEnd();
		return stdOut(pipe.getWriteEnd());
	}

	Process &Process::stdErr(SeqFileInput &stream) {
		Pipe pipe;
		stream = pipe.getReadEnd();
		return stdErr(pipe.getWriteEnd());
	}

	Process &Process::stdOutErr(SeqFileInput &stream) {
		Pipe pipe;
		stream = pipe.getReadEnd();
		return stdOutErr(pipe.getWriteEnd());
	}

	Process &Process::stdIn(SeqFileOutput &stream) {
		Pipe pipe;
		stream = pipe.getWriteEnd();
		return stdIn(pipe.getReadEnd());
	}

	Process &Process::stdOut(Process &stream) {
		Pipe pipe;
		stream.stdIn(pipe.getReadEnd());
		return stdOut(pipe.getWriteEnd());
	}

	Process &Process::stdErr(Process &stream) {
		Pipe pipe;
		stream.stdIn(pipe.getReadEnd());
		return stdErr(pipe.getWriteEnd());
	}

	Process &Process::stdOutErr(Process &stream) {
		Pipe pipe;
		stream.stdIn(pipe.getReadEnd());
		return stdOutErr(pipe.getWriteEnd());
	}

	Process &Process::stdIn(Process &stream) {
		Pipe pipe;
		stream.stdOut(pipe.getWriteEnd());
		return stdIn(pipe.getReadEnd());
	}
	Process &Process::setEnv(const ProcessEnv *env) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		ctx.curEnv = env;
		return *this;
	}

	template<typename A,typename B>
	static void escapeArg(AutoArrayStream<A,B> &buff, ConstStrA txt) {

		for (ConstStrA::Iterator iter = txt.getFwIter(); iter.hasItems();) {
			char c = iter.getNext();
			if (c == ' ' || c== '"' || c== '\\') {
				buff.write('\\');
			}
			buff.write(c);
		}

	}

	String Process::getCmdLine() const {
		AutoArrayStream<char, SmallAlloc<256> > buff;
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);

		escapeArg(buff,processName.getUtf8());
		for (natural k = 0; k < ctx.cmdLine.length(); k++) {
			buff.write(' ');
			const StringA &a = ctx.cmdLine[k];
			escapeArg(buff,a);
		}
		return String(ConstStrA(buff.getArray()));
	}


	static natural calcEnvTableSize(const ProcessEnv *env) {
		natural sz = (env->length()+1) * sizeof(char *);
		for (ProcessEnv::Iterator iter = env->getFwIter(); iter.hasItems();) {
			const ProcessEnv::Entity &e = iter.getNext();
			sz+=String::utf8length(e.key) + String::utf8length(e.value) + 2;
		}
		return sz;
	}
	static void createEnvTable(const ProcessEnv *env, char **envTable) {
		char *dataArea = (char *)(envTable+(env->length()+1));
		char **tbl = envTable;
		for (ProcessEnv::Iterator iter = env->getFwIter(); iter.hasItems();) {
			*tbl = dataArea;
			const ProcessEnv::Entity &e = iter.getNext();
			WideToUtf8Reader<ConstStrW::Iterator> rdkey(e.key.getFwIter()),rdval(e.value.getFwIter());
			while (rdkey.hasItems()) *dataArea++ = rdkey.getNext();
			*dataArea++='=';
			while (rdval.hasItems()) *dataArea++ = rdval.getNext();
			*dataArea++='\0';
			tbl++;
		}
		*tbl = 0;
	}

	static natural calcCmdLineSize(const AutoArray<StringA, RTAlloc> &cmdLine, ConstStrA processName) {
		natural sz = (cmdLine.length()+2) * sizeof(char *);
		for (AutoArray<StringA, RTAlloc>::Iterator iter = cmdLine.getFwIter(); iter.hasItems();) {
			const StringA &e = iter.getNext();
			sz+=e.length()+1;
		}
		sz+=processName.length()+1;
		return sz;

	}

	static void createCmdLine(const AutoArray<StringA, RTAlloc> &cmdL, ConstStrA processName, char **cmdLine) {
		char *dataArea = (char *)(cmdLine+(cmdL.length()+2));
		char **tbl = cmdLine;
		*tbl = dataArea;
		for (ConstStrA::Iterator iter = processName.getFwIter(); iter.hasItems();)
			*dataArea++ = iter.getNext();
		*dataArea++ = 0;
		tbl++;
		for (AutoArray<StringA, RTAlloc>::Iterator iter = cmdL.getFwIter(); iter.hasItems();) {
			*tbl = dataArea;
			const StringA &e = iter.getNext();
			for (StringA::Iterator iter = e.getFwIter(); iter.hasItems();)
				*dataArea++ = iter.getNext();
			*dataArea++ = 0;
			tbl++;
		}
		*tbl = 0;
	}

	static int getHandle(LinuxProcessContext::PStream handle) {
		if (handle == nil) return -1;
		int h;
		handle->getIfc<IFileExtractHandle>().getHandle(&h,sizeof(h));
		return h;
	}

	void Process::start() {

		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);

		//test, whether process already running
		if (ctx.pid != -1) {
			throw UnableToStartProcessException(THISLOCATION,EBUSY,"Process already running");
		}

		//prepare process name
		StringA fname = processName.getUtf8();

		//build environment table
		char **envTable;
		if (ctx.curEnv == 0) {
			//use current environment when no environment is associated
			envTable = (char **)LightSpeed::getEnv();
		} else {
			//calculate size of environment table
			natural envSz = calcEnvTableSize(ctx.curEnv);
			//allocate space
			envTable = (char **)alloca(envSz);
			//create environment table
			createEnvTable(ctx.curEnv,envTable);
		}

		//calculate size for command line
		natural cmdSz = calcCmdLineSize(ctx.cmdLine,fname);
		//allocate space
		char **cmdLine = (char **)alloca(cmdSz);
		//create command line
		createCmdLine(ctx.cmdLine,fname,cmdLine);

/*		LogObject lg(THISLOCATION);
		if (lg.isDebugEnabled()) {
			AutoArrayStream<char, SmallAlloc<256> > buff;
			char **c = cmdLine;
			while (*c != 0) {
				buff.write('"');
				buff.blockWrite(ConstStrA(*c),true);
				buff.write('"');
				buff.write(' ');
				c++;
			}

			lg.debug("Executing: %1 args %2") << fname.cStr() << ConstStrA(buff.getArray());
		}
*/

		//retrieve fds for input output and error. If not used, fd is set to -1
		int hInput = getHandle(ctx.input.get());
		int hOutput = getHandle(ctx.output.get());
		int hError = getHandle(ctx.error.get());

		StringA cwdutf8 = cwd.getUtf8();
		const char *cwdchr = cwdutf8.c_str();

		//create pipe to exec status
		Pipe ipcstatus;
		//store read end to statusRead stream
		SeqFileInput statusRead = ipcstatus.getReadEnd();
		//get pipe to write status if exec fails
		SeqFileOutput writeStatus = ipcstatus.getWriteEnd();
		//detach pipe object;
		ipcstatus.detach();

		//prepare object to read exec status
		//prepare return value from the fork
		int fres;
		//fork process
#ifndef HAVE_VFORK
#define HAVE_VFORK 0
#endif

#if HAVE_VFORK
		fres = vfork();
#else
		fres = fork();
#endif

		//fork failed?
		if (fres == -1) {
			int errn = errno;
			throw UnableToStartProcessException(THISLOCATION,errn,"<fork failure>");
		}


		//in the child process (fres = 0)
		if (fres == 0) {
			int chInput = -1, chOutput = -1, chError = -1;
			//duplicate original fds, because they are marked O_CLOEXEC
			//first duplicate to new set to handle collisions while descriptors are in 0-2 range
			if (hInput >= 0) chInput = dup(hInput);
			if (hOutput >= 0) chOutput = dup(hOutput);
			if (hError >= 0) chError = dup(hError);

			//set input output and error fds
			if (chInput >= 0) {
				dup2(chInput,0);close(chInput);
			}
			if (hOutput >= 0) {
				dup2(chOutput,1);close(chOutput);
			}
			if (hError >= 0) {
				dup2(chError,2);close(chError);
			}

			//disable O_CLOEXEC on streams exported to the child process
			for (natural i = 0; i < ctx.extraStreams.length(); i++) {
				int fd;
				ctx.extraStreams[i]->getIfc<IFileExtractHandle>().getHandle(&fd,sizeof(fd));
				fcntl(fd,F_SETFD,fcntl(fd,F_GETFD) & ~FD_CLOEXEC);
			}

			//if cwd defined, change current working directory
			int res;

			if (*cwdchr != 0) {
				res = chdir(cwdchr);
				if (res) {
					int e = errno;
					//write error code into status pipe
					writeStatus.blockWrite(&e,sizeof(e),true);

					_exit(1);
				}
			}

			//perform exec
			/* if process starts, it closes ipcstatus pipe, so parent process detect succesfull execution
			 * otherwise, function returns with exit code */
			res = execve(fname.cStr(),cmdLine,envTable);

			if (res) {
				int e = errno;
				//write error code into status pipe
				writeStatus.blockWrite(&e,sizeof(e),true);
			}
			//terminate this process
			_exit(1);
		} else {
			//parent
			//reset writeStatus, it may close becaused child already closed too
			writeStatus = nil;

			//close all fds duplicated into child
			ctx.input = nil;
			ctx.output = nil;
			ctx.error = nil;
			//close all extra streams
			ctx.extraStreams.clear();

			//wait and read status
			if (statusRead.hasItems()) {
				//if status exists
				//wait child
				int exitStatus;
				waitpid(fres,&exitStatus,0);
				int res;
				//read status
				statusRead.blockRead(&res,sizeof(res),true);
				//throw exception
				throw UnableToStartProcessException(THISLOCATION,res,getCmdLine());
			}

			//store pid of new process
			ctx.pid = fres;

			//close finish gate
			finishGate.close();
		}
	}


	void Process::stop(bool force) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		if (ctx.pid != -1) {
			if (kill(ctx.pid,force?SIGKILL:SIGTERM)) {
				int errn = errno;
				throw ErrNoWithDescException(THISLOCATION,errn,"Unable to terminate process");
			}
		}
	}


	///handles global waiting for SIGCHLD signal - ensures, that only one thread will wait
	/** Class uses event counter to detect, whether thread miss event or not
	 *
	 * Only one thread can wait for signals, other threads waits for mutex. When thread detect a signal,
	 * increases event counter and leaves mutex. Other thread enters the mutex and checks its own counter
	 * with global event counter. If differs, immediately leaves mutex and also reports event detection.
	 * After all threads receives event notification and repeats waiting request, first threads starts to
	 * wait on next event.
	 *
	 * This technique ensures, that no SIGCHLD event will be lost.
	 */
	class GlobalSigChildWait {
	public:

		GlobalSigChildWait():eventNumber(0) {}

		bool wait(Timeout wtm, integer &myEN) {

			//start cycle
			while (true) {
				//separate threads
				//first enters to guarded area to catch signal
				//other remains in waiting area waiting to signal
				if (lock.tryLock()) {
					//first check event number
					integer k = readAcquire(&eventNumber);
					//if differs
					if (k != myEN) {
						//nothing to do - store number
						myEN = k;
						lock.unlock();
						syncPt.notifyOne();
						return true;
					}

					//prepare for waiting on signal
					sigset_t sset;
					sigemptyset(&sset);
					sigaddset(&sset,SIGCHLD);
					//calculate timeout
					struct timespec tmsp = wtm.getRemain().getTimeSpec();
					bool res;
					while (true) {
						//wait for signal
						int x = sigtimedwait(&sset,0,&tmsp);
						//handle error
						if (x == -1) {
							int e = errno;
							//repeat on EINTR
							if (e == EINTR) continue;
							//EAGAIN - timeout
							else if (e == EAGAIN) {
								//remember false result
								res = false;
								//unlock the lock before releasing thread
								lock.unlock();
								//let another thread watching signals
								syncPt.notifyOne();
								//exit cycle
								break;
							} else {

								//unlock guarded section
								lock.unlock();
								//give stick to another thread
								syncPt.notifyOne();
								//throw exception
								throw ErrNoException(THISLOCATION,e);
							}
						} else {
							//rembemer true result
							res = true;
							//increase event number
							lockInc(eventNumber);
							//update myEN
							myEN = eventNumber;
							//unlock the lock before releasing threads
							lock.unlock();
							//event number has been changed - notify all threads
							syncPt.notifyAll();
							//leave cycle
							break;
						}
					}
					return res;

				} else {
					//read current event number
					integer k = readAcquire(&eventNumber);
					//if differs
					if (k != myEN) {
						//store it
						myEN = k;
						//and returns true, because event number is different
						return true;
					}
					//create slot
					SyncPt::Slot s;
					//add slot to the sync point
					syncPt.add(s);
					//start waiting if number did not changed
					if (readAcquire(&eventNumber) != myEN) syncPt.wait(s,wtm);
					//remove slot from syncPt (may not be signaled)
					syncPt.remove(s);
					//check event number again
					k = readAcquire(&eventNumber);
					//in case, that differs
					if (k != myEN) {
						//update it
						myEN = k;
						//success
						return true;
					}

					//if slot is not signaled, exit with unsuccessful attempt
					if (!s) return false;
					//otherwise, repeat whole cycle (may change to enter guard section)
				}

			}
#if 0
			//perform locked LLEventMutex
			Synchronized<LLEventMutex> _(masterLock);
			//check eventNumber
			if (myEN != eventNumber) {
				//if differ, event has been detected from last check - remember new event counter
				myEN = eventNumber;
				//return true;
				return true;
			}
			//wait for event
			if (masterLock.waitLocked(wtm) == false) {
				//if timeout, exit false
				return false;
			}

			//check for event counter now
			if (myEN != eventNumber) {
				//if differ, remember new value
				myEN = eventNumber;
				//open event for next thread
				masterLock.notify();
				//return success
				return true;
			}

			//prepare for waiting on signal
			sigset_t sset;
			sigemptyset(&sset);
			sigaddset(&sset,SIGCHLD);
			//calculate timeout
			struct timespec tmsp = wtm.getRemain().getTimeSpec();
			bool res;
			while (true) {
				//wait for signal
				int x = sigtimedwait(&sset,0,&tmsp);
				//handle error
				if (x == -1) {
					int e = errno;
					//repeat on EINTR
					if (e == EINTR) continue;
					//EAGAIN - timeout
					else if (e == EAGAIN) {
						//remember false result
						res = false;
						//exit cycle
						break;
					} else {
						//allow next thread run
						masterLock.notify();
						//throw exception
						throw ErrNoException(THISLOCATION,e);
					}
				} else {
					//rembemer true result
					res = true;
					//increase event number
					eventNumber++;
					//update myEN
					myEN = eventNumber;
					//leave cycle
					break;
				}
			}
			//release next thread
			masterLock.notify();
			//return result
			return res;
#endif
			}

	protected:
//		LLEventMutex masterLock;
		SpinLock lock;
		SyncPt syncPt;
		atomic eventNumber;

	};

	///using waitpid checks whether process is running
	/**
	 * @param ctx process context
	 * @param flags should be 0 to normal wait, or WNOHANG to checking without waiting
	 * @return function returns positive value as exitCode, negative value as term signal. Function returns -256
	 * 			when flags is WNOHANG and process still runninf
	 */
	static integer processWaitPid(LinuxProcessContext &ctx, int flags) {
		//process has been started?
		if (ctx.pid != -1) {
			int status;
			do {
				//perform waiting on process
				int r = waitpid(ctx.pid,&status,flags);
				//if error
				if (r == -1) {
					//check error
					int e = errno;
					//if error is EINTR, repeat waiting
					if (e == EINTR) continue;
					//otherwise, throw exception
					else throw ErrNoException(THISLOCATION,e);
				} if (r == 0) {
					//if zero returned, then WNOHANG in effect, return -256
					return -256;
				} else {
					//determine status of exited process
					if (WIFEXITED(status)) {
						//store exit value
						ctx.exitCode = WEXITSTATUS(status);
					} else if (WIFSIGNALED(status)) {
						//store term signal
						ctx.exitCode = -(WTERMSIG(status));
					}
					//process exited, set not started
					ctx.pid = -1;
					//return exit code
					return ctx.exitCode;
				}
			} while (true);
		} else {
			//process has not been started
			//return last known exit code
			return ctx.exitCode;
		}
		throw;

	}

	integer Process::join() {

		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		//wait for process
		integer res = processWaitPid(ctx,0);
		//open finish gate other threads may continue
		finishGate.open();
		//reset cmdline and prepare for next run
		ctx.cmdLine.clear();
		//return exit code
		return res;
	}

	bool Process::join(const Timeout &tm) {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		//if process has not been started, return with true
		/* Join is always successful, if process is not running */
		if (ctx.pid == -1) return true;

		//determine status of the process
		integer res = processWaitPid(ctx,WNOHANG);
		integer myEN = 0;
		//if process is not running
		while (res == -256) {
			//acquire GlobalSigChildWait signleton object
			GlobalSigChildWait &sigchldwt = Singleton<GlobalSigChildWait>::getInstance();
			//perform waiting on signal
			if (!sigchldwt.wait(tm,myEN)) {
				//if timeouted, exit
				return false;
			}
			//global event detected, but is this our event?
			//recheck the situation
			res = processWaitPid(ctx,WNOHANG);

		}

		//open finish gate other threads may continue

		finishGate.open();
		//reset cmdline and prepare for next run
		ctx.cmdLine.clear();
		return true;
	}

	void Process::detach() {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		ctx.pid = -1;
		ctx.exitCode = -1;
	}


	integer Process::getExitCode() const {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		return ctx.exitCode;
	}

	bool Process::isRunning() const {
		LinuxProcessContext &ctx = LinuxProcessContext::getCtx(*context);
		return ctx.pid != -1;
	}



	class GlobalProcessEnv: public ProcessEnv {
	public:
		GlobalProcessEnv() {

			typedef Map<StringPoolW::Str, StringPoolW::Str, ProcessEnvKeyCompare> ProcessEnvTmp;
			ProcessEnvTmp envTmp;

			char const* const* env = getEnv();
			while (*env) {
				const char *line = *env;
				const char *eqp = strchr(line,'=');
				if (eqp) {

					envTmp.insert(addpool(ConstStrA(line,eqp-line)),addpool(ConstStrA(eqp+1)));
				}

				env++;
			}
			for (ProcessEnvTmp::Iterator iter = envTmp.getFwIter(); iter.hasItems(); ) {
				const ProcessEnvTmp::Entity &e = iter.getNext();
				ProcessEnv::insert(ConstStrW(e.key),ConstStrW(e.value));
			}
		}

		StringPoolW::Str addpool(ConstStrA str) {
			Utf8ToWideReader<ConstStrA::Iterator> rd(str.getFwIter());
			return envPool.add(rd);
		}
		StringPoolW envPool;

	};

	ProcessEnv Process::getEnv() {
		GlobalProcessEnv &globenv = Singleton<GlobalProcessEnv>::getInstance();
		return globenv;
	}


}


#if 0

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/wait.h>
#include <alloca.h>
#include "../../base/containers/string.h"
#include "../../base/memory/rtAlloc.h"
#include "../../base/exceptions/invalidParamException.h"
#include "../../base/streams/utf.h"
#include "../../base/exceptions/fileExceptions.h"
#include "../process.h"
#include "../gate.h"

#include "../../base/iter/iteratorFilter.tcc"
#include "../../base/containers/autoArray.tcc"
#include "../../mt/thread.h"


namespace LightSpeed {

	class ProcessContext;
	void afterWait(ProcessContext &ctx, pid_t e, int status);


	class ProcessContext: public IProcessContext {
	public:

		pid_t processPid;

		int processInput;
		int processOutput;
		int processError;
		int exitCode;
		bool crashed;
		int startError;

		AutoArray<String, RTAlloc > cmdLine;

		const ProcessEnv *env;



		ProcessContext(IRuntimeAlloc *alloc):processPid(-1),
				processInput(-1),processOutput(-1),
				processError(-1),exitCode(0),crashed(false),
				cmdLine(*alloc),
				env(0) {}


		static ProcessContext &getCtx(IProcessContext &ctx) {
			return static_cast<ProcessContext &>(ctx);
		}
		static const ProcessContext &getCtx(const IProcessContext &ctx) {
			return static_cast<const ProcessContext &>(ctx);
		}

		void closeInput() {
			if (processInput != -1) {
				close(processInput);
				processInput = -1;
			}
		}

		void closeOutput() {
			if (processOutput != -1) {
				close(processOutput);
				processOutput = -1;
			}
		}

		void closeError() {
			if (processError != -1) {
				close(processError);
				processError = -1;
			}
		}
		~ProcessContext() {
			closeInput();closeOutput();closeError();

			if (processPid != -1) {
				int status;
				pid_t p = waitpid(processPid,&status,0);
				afterWait(*this,p,status);
			}
		}

		void setCmdLine(const ProcessCmdLine &cl) {

			cmdLine.clear();
			cmdLine.reserve(cl.length());
			for (ProcessCmdLine::Iterator iter = cl.getFwIter(); iter.hasItems();) {
				cmdLine.add(iter.getNext());
			}
		}
	};


	Process::Process()
		:context(new(RTFactory(StdAlloc::getInstance()))
				PolyAlloc<RTFactory,ProcessContext>(&StdAlloc::getInstance())) {
	}
	Process::Process(const ProcessCmdLine &cmdLine)
		:context(new(RTFactory(StdAlloc::getInstance()))
				PolyAlloc<RTFactory,ProcessContext>(&StdAlloc::getInstance())) {
		setCmd(cmdLine);
	}
	Process::Process(IRuntimeAlloc &alloc)
		:context(new(RTFactory(alloc))
				PolyAlloc<RTFactory,ProcessContext>(&alloc)) {
	}
	Process::Process(const ProcessCmdLine &cmdLine,IRuntimeAlloc &alloc)
		:context(new(RTFactory(alloc))
				PolyAlloc<RTFactory,ProcessContext>(&alloc)) {
		setCmd(cmdLine);
	}

    int pipeCloseOnExec(int *fds);

	Process &Process::operator | (Process &other) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ProcessContext &octx = ProcessContext::getCtx(*other.context);
		int fds[2];
		int e = pipeCloseOnExec(fds);
		if (e == -1)
			throw ErrNoException(THISLOCATION, errno);

		ctx.closeOutput();
		octx.closeInput();
		octx.processInput = fds[0];
		ctx.processOutput = fds[1];
		return other;
	}

	Process &Process::operator || (Process &other) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ProcessContext &octx = ProcessContext::getCtx(*other.context);
		int fds[2];
		int e = pipeCloseOnExec(fds);
		if (e == -1)
			throw ErrNoException(THISLOCATION, errno);

		ctx.closeOutput();
		octx.closeInput();
		octx.processInput = fds[0];
		ctx.processError = fds[1];
		return other;
	}

	int getFdFromStream(PSeqFileHandle hndl, const ProgramLocation &loc) {
		IFileExtractHandle *fdinfo = dynamic_cast<IFileExtractHandle *>(hndl.get());
		if (fdinfo == 0) {
			throw InvalidParamException(loc,1,"Object must be native file, pipe or socket");
		}
		int fd;
		try {
			fdinfo->getHandle(&fd,sizeof(fd));
		} catch (Exception &e) {
			throw InvalidParamException(loc,1,"Object must be based on Posix Descriptor") << e;
		}
		return fd;
	}

	Process &Process::operator < (const SeqFileInput &input) {
		int fd = getFdFromStream(input.getHandle(),THISLOCATION);
		int newfd = dup(fd);
		if (newfd == -1)
			throw ErrNoException(THISLOCATION, errno);
		fcntl(newfd,F_SETFD, FD_CLOEXEC);
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.closeInput();
		ctx.processInput = newfd;
		return *this;
	}
	Process &Process::operator < (SeqFileOutput &output) {
		IFileIOServices &svc = IFileIOServices::getIOServices();
		PSeqFileHandle re,we;
		svc.createPipe(re,we);
		output = SeqFileOutput(we);
		return operator < (SeqFileInput(re));
	}
	Process &Process::operator > (const SeqFileOutput &output) {
		int fd = getFdFromStream(output.getHandle(),THISLOCATION);
		int newfd = dup(fd);
		if (newfd == -1)
			throw ErrNoException(THISLOCATION, errno);
		fcntl(newfd,F_SETFD, FD_CLOEXEC);
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.closeOutput();
		ctx.processOutput = newfd;
		return *this;
	}
	Process &Process::operator > (SeqFileInput &output) {
		IFileIOServices &svc = IFileIOServices::getIOServices();
		PSeqFileHandle re,we;
		svc.createPipe(re,we);
		output = SeqFileInput(re);
		return operator > (SeqFileOutput(we));
	}
	Process &Process::operator >> (const SeqFileOutput &output) {
		int fd = getFdFromStream(output.getHandle(),THISLOCATION);
		int newfd = dup(fd);
		if (newfd == -1)
			throw ErrNoException(THISLOCATION, errno);
		fcntl(newfd,F_SETFD, FD_CLOEXEC);
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.closeError();
		ctx.processError = newfd;
		return *this;
	}
	Process &Process::operator >> (SeqFileInput &output) {
		IFileIOServices &svc = IFileIOServices::getIOServices();
		PSeqFileHandle re,we;
		svc.createPipe(re,we);
		output = SeqFileInput(re);
		return operator >> (SeqFileOutput(we));
	}
	Process &Process::setCmd(const ProcessCmdLine &cmdLine) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.setCmdLine(cmdLine);
		return *this;
	}


	void Process::setEnv(const ProcessEnv *env) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.env = env;

	}

	natural createCmdLine(char **cmdLine, const ArrayRef<String> &arrLine) {

		char *wrp = (char *) cmdLine;
		natural ptr = (arrLine.length() + 1) * sizeof(char *);
		for (ArrayRef<String>::Iterator iter = arrLine.getFwIter(); iter.hasItems();) {
			const String &p = iter.getNext();
			if (cmdLine) {
				*cmdLine = wrp + ptr;
				cmdLine++;
			}
			WideToUtf8Reader<String::Iterator> rd(p.getFwIter());
			while (rd.hasItems()) {
				char c = rd.getNext();
				if (wrp) wrp[ptr] = c;
				ptr++;
			}

			if (wrp) wrp[ptr] = 0;
			ptr++;
		}

		if (cmdLine) *cmdLine = 0;
		return ptr;
	}

	natural createEnv(char **envLine, const ProcessEnv *env) {

		if (env == 0) {
			if (envLine != 0) *envLine = 0;
			return sizeof(char *);
		}

		char *wrp = (char *) envLine;
		natural ptr = (env->size() + 1) * sizeof(char *);
		for (ProcessEnv::Iterator iter = env->getFwIter(); iter.hasItems();) {
			const ProcessEnv::Entity &p = iter.getNext();
			if (envLine) {
				*envLine = wrp + ptr;
				envLine++;
			}
			WideToUtf8Reader<String::Iterator> k(p.key.getFwIter()),v(p.value.getFwIter());
			while (k.hasItems()) {
				char c = k.getNext();
				if (wrp) wrp[ptr] = c;
				ptr++;
			}
			if (wrp) wrp[ptr] = '=';
			ptr++;
			while (v.hasItems()) {
				char c = v.getNext();
				if (wrp) wrp[ptr] = c;
				ptr++;
			}
			if (wrp) wrp[ptr] = 0;
			ptr++;
		}

		if (envLine) *envLine = 0;
		return ptr;
	}

	void testFileRunnable( String fname, const ProgramLocation &loc) {
		ArrayRef<String> pp(&fname,1);
		natural needsz = createCmdLine(0,pp);
		char **buff = (char **)alloca(needsz);
		createCmdLine(buff,pp);
		if (access(buff[0],X_OK) != 0)
			throw FileOpenError(THISLOCATION,errno,fname);
	}

	void Process::start() {

		ProcessContext &ctx = ProcessContext::getCtx(*context);
		if (ctx.processPid != -1) return;
		testFileRunnable(ctx.cmdLine[0],THISLOCATION);

		finishGate.close();
		ctx.startError = 0;


		int fres = vfork();
		if (fres == -1) {
			finishGate.open();
			throw ErrNoException(THISLOCATION, errno);
		}

		if (fres == 0) {

			if (ctx.processInput != -1) {
				dup2(ctx.processInput,0);
			}
			if (ctx.processOutput != -1) {
				dup2(ctx.processOutput,1);
			}
			if (ctx.processError != -1) {
				dup2(ctx.processError,2);
			}

			natural needsz = createCmdLine(0,ctx.cmdLine);
			char **ccmdline = (char **)alloca(needsz);
			createCmdLine(ccmdline,ctx.cmdLine);

			needsz = createEnv(0,ctx.env);
			char **cenv = (char **)alloca(needsz);
			createEnv(cenv,ctx.env);

			execve(ccmdline[0],ccmdline,cenv);

			ctx.startError = errno;

			_exit(ctx.startError);
		}

		ctx.closeInput();
		ctx.closeOutput();
		ctx.closeError();

		int status;
		pid_t p = waitpid(fres,&status,WNOHANG);
		if (p != 0) {
			afterWait(ctx,p,status);

			if (ctx.startError)
				throw ErrNoWithDescException(THISLOCATION,ctx.startError,
						"Process creation has failed");

		} else {
			ctx.processPid = fres;
		}

	}


	void afterWait(ProcessContext &ctx, pid_t e, int status, Gate &finishGate) {
		if (e == -1) throw ErrNoException(THISLOCATION,errno);
		if (WIFEXITED(status)) {
			ctx.crashed = false;
			ctx.exitCode = WEXITSTATUS(status);
		} else if (WIFSIGNALED(status)) {
			ctx.crashed = true;
			ctx.exitCode = WTERMSIG(status);
		}
		finishGate.open();
		ctx.processPid = -1;
	}

	void Process::join() {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		if (ctx.processPid != -1) {
			int status;
			pid_t e = waitpid(ctx.processPid,&status,0);
			afterWait(ctx,e,status);
		}
	}

	bool Process::join(const Timeout &tm) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		if (ctx.processPid != -1) {
			natural tmms = 1;
			int status;
			pid_t e = waitpid(ctx.processPid,&status, WNOHANG);
			while (e == 0) {
				if (tm.expired()) return false;
				Thread::sleep(tmms);
				if (tmms < 500) tmms++;
				e = waitpid(ctx.processPid,&status, WNOHANG);
			}
			afterWait(ctx,e,status);
		}
		return true;

	}

	bool Process::isRunning() const {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		if (ctx.processPid != -1) {
			if (kill(ctx.processPid,0) == 0)
				return true;
		}
		return false;

	}




	void Process::stop(bool force) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		if (ctx.processPid == -1) return;
		if (force) ::kill(ctx.processPid,SIGKILL);
		else ::kill(ctx.processPid,SIGTERM);
	}

	void Process::detach() {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.processPid = -1;
	}

	integer Process::getExitCode() const {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		if (ctx.crashed) return -ctx.exitCode;
		else return ctx.exitCode;
	}

	Process::Process(const ConstStrW &cmdLine)
	:context(new(RTFactory(StdAlloc::getInstance()))
			PolyAlloc<RTFactory,ProcessContext>(&StdAlloc::getInstance())) {
	setCmd(cmdLine);
	}

	Process::Process(const ConstStrW &cmdLine, IRuntimeAlloc &alloc)
	:context(new(RTFactory(alloc))
			PolyAlloc<RTFactory,ProcessContext>(&alloc)) {
	setCmd(cmdLine);

	}

	Process &Process::setCmd(const ConstStrW &cmdLine) {
		ProcessContext &ctx = ProcessContext::getCtx(*context);
		ctx.cmdLine.clear();
		AutoArrayStream<wchar_t> buff;

		for (ConstStrW::Iterator iter = cmdLine.getFwIter(); iter.hasItems();) {
			wchar_t z = iter.getNext();
			if (z == '"') {
				while (iter.hasItems()) {
					z = iter.getNext();
					if (z == '"') {
						if (iter.hasItems() && iter.peek() == '"') {
							buff.write(iter.getNext());
						} else {
							break;
						}
					} else {
						buff.write(z);
					}
				}
			} else if (z == ' ') {

				if (!buff.empty()) {
					ctx.cmdLine.add(String(buff.getArray()));
					buff.clear();
				}
			} else {
				buff.write(z);
			}
		}
		if (!buff.empty()) {
			ctx.cmdLine.add(String(buff.getArray()));
		}
		return *this;
	}

}
#endif
