/** @file
 * Copyright (c) 2006, Seznam.cz, a.s.
 * All Rights Reserved.
 * 
 * $Id: proginstance.cc 722 2015-07-13 13:05:40Z bredysoft $
 *
 * DESCRIPTION
 * Short description
 * 
 * AUTHOR
 * Ondrej Novak <ondrej.novak@firma.seznam.cz>
 *
 */

#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include "../framework/proginstance.h"
#include "../exceptions/systemException.h"
#include "../containers/string.tcc"
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <wait.h>
#include <signal.h>
#include <sys/signal.h>
#include <malloc.h>
#include "../debug/dbglog.h"
#include "../streams/fileio.h"
#include "../streams/fileiobuff.tcc"
#include "../interface.tcc"


namespace LightSpeed {

static natural restartCount = 0;
static time_t startTime, restartTime;


struct ProgInstanceBuffer {
	///owner of this file
	pid_t pid;
	///thread id of the owner
	pthread_t tid;
	///pointer to owner - used only internally to identify master object
	ProgInstance *owner;
	///size of this file (including pid)
	natural size;
	///size reserved for request
	natural requestSize;
	///size of current request
	natural curRequestSize;
	///accessing the buffer by caller
	sem_t semAccess;
	///triggered after request is placed
	sem_t semRequest;
	///locked when processing the request
	sem_t semInProgress;
	///triggered after reply is placed
	sem_t semReply;
	///in daemon mode
	bool indaemon;
	///data reserved for request/reply
	char data[1];
};


ProgInstance::ProgInstance(const String &name):name(name) {
	buffer = 0;
	initializeStartTime();
}

ProgInstance::ProgInstance(const ProgInstance &other):name(other.name) {
	buffer = 0;
	initializeStartTime();
}


ProgInstance::~ProgInstance() {
	close();
}

void ProgInstance::create()
{
	long pgsize = sysconf(_SC_PAGESIZE);
	long requestSize = pgsize - sizeof(ProgInstanceBuffer);
	return create(requestSize);
}

void ProgInstance::open()
{
	if (buffer != 0) return;
	int fd = ::open(name.getUtf8().c_str(),O_RDWR);
	if (fd == -1)
		throw NotRunningException(THISLOCATION);
	struct stat statbuf;
	if (fstat(fd,&statbuf) != 0) {
		::close(fd);
		throw ErrNoException(THISLOCATION, errno);
	}

	void *mb = mmap(0,statbuf.st_size,PROT_WRITE|PROT_READ,MAP_SHARED,fd,0);
	::close(fd);
	if (mb == MAP_FAILED)
		throw ErrNoException(THISLOCATION, errno);

	ProgInstanceBuffer *buf = reinterpret_cast<ProgInstanceBuffer *>(mb);
	if (kill(buf->pid,0) != 0) {
		throw NotRunningException(THISLOCATION);
	}

	buffer = buf;
}

bool ProgInstance::anyRequest()
{
	//not initialized - error
	if (buffer == 0)
		throw NotInitialized(THISLOCATION);
	//try to wait for semaphore
	if (sem_trywait(&buffer->semRequest)) {
		//if not success - probably no request
		return false;
	}
	//successed, but we don't want to accept request now - post the semaphore
	sem_post(&buffer->semRequest);
	return true;

}

bool ProgInstance::check() const {
	IFileIOServices &svc = IFileIOServices::getIOServices();
	if (svc.canOpenFile(name,IFileIOServices::fileAccessible))
		return svc.canOpenFile(name,IFileIOServices::fileOpenReadWrite);
	natural lastSlash = name.findLast('/');
	if (lastSlash == naturalNull) return !name.empty() && svc.canOpenFile('.',IFileIOServices::fileOpenReadWrite);
	ConstStrW path = name.head(lastSlash);
	return svc.canOpenFile(path,IFileIOServices::fileOpenReadWrite);
}

static void sem_throwException(const ProgramLocation &loc, int err) {
	if (err == EINTR) throw ProgInstance::InterruptedWaitingException(loc,err);
	else throw ErrNoException(loc,err);
}

static bool sem_wait(sem_t &sem,natural timeout) {

	if (timeout == naturalNull) {
		if (sem_wait(&sem)) {
			sem_throwException(THISLOCATION, errno);
		} return true;
	}

	struct timeval tmval;
	gettimeofday(&tmval,0);
	tmval.tv_usec += (timeout % 1000)*1000;
	tmval.tv_sec += timeout / 1000;
	if (tmval.tv_usec >= 1000000) {
		tmval.tv_sec++;
		tmval.tv_usec -= 1000000;
	}

	struct timespec tmspec;
	tmspec.tv_sec = tmval.tv_sec;
	tmspec.tv_nsec = tmval.tv_usec * 1000;

	int i = sem_timedwait(&sem,&tmspec);
	if (i == -1) {
		int e = errno;
		if (e == EAGAIN || e==ETIMEDOUT) return false;
		else sem_throwException(THISLOCATION,errno);
	}
	return true;
}

void *ProgInstance::waitForRequest(natural timeout)
{
	//not initialized - error
	if (buffer == 0)
		throw NotInitialized(THISLOCATION);

	if (sem_wait(buffer->semRequest,timeout)) {
		if (buffer->curRequestSize == 0) {
			sendReply(0,0);
			return 0;
		}
		else {
			return buffer->data;
		}
	} else {
		return 0;
	}
}

natural ProgInstance::getReplyMaxSize()
{
	//not initialized - error
	if (buffer == 0)
		throw NotInitialized(THISLOCATION);

	return buffer->requestSize;
}

void ProgInstance::create(natural requestMaxSize)
{

	StringA namea = name.getUtf8();
	if (buffer != 0) return;
	natural totalSize = requestMaxSize + sizeof(ProgInstanceBuffer);
	int fd = ::open(namea.c_str(),O_RDWR);
	if (fd != -1) {

		void *mb = mmap(0,sizeof(ProgInstanceBuffer),PROT_READ,MAP_SHARED,fd,0);
		::close(fd);
		if (mb == MAP_FAILED)
			throw ErrNoException(THISLOCATION,errno);

		ProgInstanceBuffer *buf = reinterpret_cast<ProgInstanceBuffer *>(mb);
		if (kill(buf->pid,0) == 0) {
			throw AlreadyRunningException(THISLOCATION,buf->pid);
		}

		munmap(mb,sizeof(ProgInstanceBuffer));
		unlink(namea.c_str());
	}
	fd = ::open(namea.c_str(),O_RDWR | O_CREAT | O_EXCL, S_IRUSR|S_IWUSR );
	if (fd == -1)
		throw ErrNoException(THISLOCATION,errno);

	char prebuf[256];
	natural cursize = 0;
	while (cursize < totalSize) {
		natural c = (totalSize - cursize) > 256?256:(totalSize - cursize);
		natural n = write(fd,prebuf,c);
		cursize +=n;
	}

	void *mb = mmap(0,totalSize,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	::close(fd);
	if (mb == MAP_FAILED)
		throw ErrNoException(THISLOCATION,errno);

	buffer =reinterpret_cast<ProgInstanceBuffer *>(mb);


	buffer->requestSize = requestMaxSize;
	buffer->pid = getpid();
	buffer->tid = pthread_self();
	buffer->owner = this;
	sem_init(&buffer->semAccess,1,1);
	sem_init(&buffer->semRequest,1,0);
	sem_init(&buffer->semReply,1,0);
	sem_init(&buffer->semInProgress,1,1);
	buffer->size = totalSize;
	buffer->indaemon = false;
}

natural ProgInstance::request(const void *req, natural reqsize, void *reply, natural replySize,natural timeout)
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	if (reqsize > buffer->requestSize) throw RequestTooLarge(THISLOCATION);
	//wait for accessing the buffer
	//there can be another client waiting for reply.
	//check for timeout and throw exception, if operation takes long
	if (!sem_wait(buffer->semAccess,timeout)) throw TimeoutException(THISLOCATION);
	try {
		//for placing request, we must also lock the buffer
		//because there can be also server, which currently processing
		//the request of client, which previously given up the waiting
		if (!sem_wait(buffer->semInProgress,timeout))
			throw TimeoutException(THISLOCATION);
		//reset any stored requests, we did not placed any yet
		while (sem_trywait(&buffer->semRequest) == 0) {}
		//reset any stored reply, we don't expect any yet
		while (sem_trywait(&buffer->semReply) == 0) {}
		//all semaphores reset and no one has chance to change this
		//now place the request data
		memcpy(buffer->data, req, reqsize);
		//set request size
		buffer->curRequestSize = reqsize;
		//release semInProgress, we don't need access the buffer anymore
		sem_post(&buffer->semInProgress);
		//notify server, that request is available
		sem_post(&buffer->semRequest);
		//now, wait for reply
		if (!sem_wait(buffer->semReply,timeout))
			//in case of timeout, we give up
			//any semaphore state leaved, this is job of next client
						throw TimeoutException(THISLOCATION);
		//check reply size
		if (buffer->curRequestSize > replySize) throw ReplyTooLarge(THISLOCATION);
		//we can get data now, without need locking
		//because server did not access the buffer until new request is placed
		//and no-one can accesss the buffer now
		memcpy(reply,buffer->data, buffer->curRequestSize);
		natural result = buffer->curRequestSize;
		//unlock the whole access to allow next client enter
		sem_post(&buffer->semAccess);
		//send result
		return result;
	} catch (...) {
		//in case of any exception
		//release access lock
		sem_post(&buffer->semAccess);
		//rethrow exception
		throw;
	}

}

natural ProgInstance::getRequestMaxSize()
{
	if (buffer == 0) return 0;
	return buffer->requestSize;
}

void *ProgInstance::acceptRequest()
{
	if (buffer == 0) return 0;
	///test, whether there is request
	if (sem_trywait(&buffer->semRequest)) return 0;
	///if there is request, lock the buffer
	::sem_wait(&buffer->semInProgress);
	///not, we can work with the data
	return buffer->data;
}

void ProgInstance::sendReply(const void *data, natural sz)
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	if (sz > buffer->requestSize) throw ReplyTooLarge(THISLOCATION);
	memcpy(buffer->data,data,sz);
	buffer->curRequestSize = sz;
	///unlock the buffer
	sem_post(&buffer->semInProgress);
	///trigger reply
	sem_post(&buffer->semReply);
}

void ProgInstance::close() {
	if (buffer) {
		bool cleanup = imOwner();
		munmap(buffer,buffer->size);
		if (cleanup)
			unlink(name.getUtf8().c_str());
		buffer = 0;
	}

}


void *ProgInstance::getReplyBuffer() {
	return buffer->data;
}
void ProgInstance::sendReply(natural sz)
{
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	if (sz > buffer->requestSize) throw ReplyTooLarge(THISLOCATION);
	buffer->curRequestSize = sz;
	//unlick the buffer
	sem_post(&buffer->semInProgress);
	//trigger reply
	sem_post(&buffer->semReply);
}


bool ProgInstance::imOwner() const {
	return buffer && buffer->pid == getpid() && buffer->owner == this;
}

bool ProgInstance::takeOwnership() {
	if (buffer->owner == this) {
		buffer->pid = getpid();
		buffer->tid = pthread_self();
		return true;
	} else {
		return false;
	}
}

void ProgInstance::TimeoutException::message(ExceptionMsg &msg) const {
	msg("Service response timeout");
}
void ProgInstance::RequestTooLarge::message(ExceptionMsg &msg) const {
	msg("Request too large");
}
void ProgInstance::ReplyTooLarge::message(ExceptionMsg &msg) const {
	msg("Reply too large");
}
void ProgInstance::AlreadyRunningException::message(ExceptionMsg &msg) const {
	msg("Service is already running under pid: %1") << pid;
}
void ProgInstance::NotRunningException::message(ExceptionMsg &msg) const {
	msg("Service is not running");
}

void ProgInstance::NotInitialized::message(ExceptionMsg &msg) const {
	msg("Service is not initialized");
}

static void waitForTerminateSt(natural timeout, pid_t pid);

void ProgInstance::terminate() {
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	pid_t pid = buffer->pid;
	kill(pid,SIGTERM);
	try {
		waitForTerminateSt(3000,pid);
	} catch (const TimeoutException &e) {
		kill(pid,SIGKILL);
		waitForTerminateSt(3000,pid);
	}
}


void ProgInstance::waitForTerminate(natural timeout) {
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	pid_t pid = buffer->pid;
	close();
	waitForTerminateSt(timeout,pid);
}

static void waitForTerminateSt(natural timeout, pid_t pid) {
	natural waits = 0;
	timeout *=2;
	while (waits < timeout) {
		if (kill(pid,0) != 0) return;
		usleep(500000);
		waits+=500;
	}
	if (kill(pid,0) != 0) return;
	throw ProgInstance::TimeoutException(THISLOCATION);
}

void ProgInstance::InterruptedWaitingException::message(ExceptionMsg &msg) const {
	msg("Unhandled interrupted waiting (EINTR)");
}

void ProgInstance::cancelWaitForRequest() {
	if (buffer == 0) throw NotInitialized(THISLOCATION);
	if (pthread_self() == buffer->tid) return;
	request(0,0,0,0,0);
}

bool ProgInstance::inDaemonMode() const {
	return buffer != 0 && imOwner() && buffer->indaemon;
}


ProgInstance::InstanceStage ProgInstance::getInstanceStage() const {
	if (buffer == 0) return standard;
	if (imOwner()) return controller;
	if (inDaemonMode()) return daemon;
	return standard;
}


natural ProgInstance::getRestartCounts()  {
	return restartCount;
}
natural ProgInstance::getUpTime(bool resetOnRestart) {
    time_t curTime;
	time(&curTime);
	if (resetOnRestart) return curTime - restartTime;
	else return curTime - startTime;
}

class ForkAndForwardIO {
public:

	ForkAndForwardIO() {}

	int doFork() {

		Pipe p;
		inp = Constructor1<SeqFileInBuff<>, SeqFileInput>(p.getReadEnd());
		SeqFileOutput oup = p.getWriteEnd();
		int forkRes = fork();

		if (forkRes == -1) throw ErrNoException(THISLOCATION,errno);;
		if (forkRes == 0) {
			int fd;
			oup.getStream()->getIfc<IFileExtractHandle>().getHandle(&fd,sizeof(fd));
			dup2(fd,1);
			dup2(fd,2);
		}
		return forkRes;
	}

	ConstStrA readLine() {
		linebuff.clear();
		byte b;
		while (inp->hasItems()) {
			b = inp->getNext();
			if (b == '\n') break;
			linebuff.write((char)b);
		}
		return linebuff.getArray();
	}

	bool hasItems() const {return inp->hasItems();}


protected:
	Optional<SeqFileInBuff<> > inp;
	AutoArrayStream<char> linebuff;
};

void ProgInstance::enterDaemonMode(natural restartOnErrorSec) {
	LogObject lg(THISLOCATION);
	if (!imOwner()) throw NotInitialized(THISLOCATION);
	int err = ::daemon(1,1);
	if (err == -1) throw ErrNoException(THISLOCATION,errno);

	initializeStartTime();

	lg.debug("ResartOnErrorSec:%1")<<restartOnErrorSec;
	if (restartOnErrorSec != 0) {
	    LogObject lg(THISLOCATION);
	    lg.note("Entering daemon with restarting feature enabled - delay: %1") << restartOnErrorSec;
	    restartCount = 0;
	    time_t curTime;

	    ForkAndForwardIO fafio;

	    int k = fafio.doFork();
	    while (k != 0) {
	    	DbgLog::setThreadName("watchdog",false);
			if (k == -1) throw ErrNoException(THISLOCATION,errno);
			int status;
			//wait pid no hang
			err = waitpid(k,&status,WNOHANG);
			while(err == 0) {
				//if still running, read input blocking (should close on exit or crash)
				ConstStrA line = fafio.readLine();
				//test running status again
				err = waitpid(k,&status,WNOHANG);
				//if line is empty and there are no items...
				if (line.empty() && !fafio.hasItems()) {
					//if no exit yet caught
					if (err == 0) {
						//input has been probably closed, we need to block on waiting
						err = waitpid(k,&status,0);
					}
					//in case on other result of wait cycle should stop here
				} else {
					//rotate logs (because we don't have information about rotation, reopen log for every line
					DbgLog::logRotate();
					//put line to log
					lg.warning("%1") << line;
					//repeat cycle and continue waiting
				}
			}
			//read any unprocessed output resides in the buffer
			while (fafio.hasItems()) {
				ConstStrA line = fafio.readLine();
				lg.warning("%1") << line;
			}
			//Finally, rotate logs
			DbgLog::logRotateAll();
			//measure time
			time(&curTime);
			if (err == -1) throw ErrNoException(THISLOCATION,errno);
			int sig = 0;
			if (WIFEXITED(status)) {
				int exitcode = WEXITSTATUS(status);
				lg.note("Process exited with status %1") << exitcode;
				if ( exitcode != 251) {
					_exit(exitcode);
				}
			} else {
				sig =WTERMSIG(status);
				if ((curTime - startTime) < 5) {
					lg.error("Process exited with signal %1 - will no restarted, because it crashed too soon") << sig;
					_exit(sig);
				}
				//perform some logging
				lg.error("Process crashed with signal %1, restarting (after delay)") << sig;
				natural runTm = curTime - restartTime;
				if (runTm < restartOnErrorSec) sleep(restartOnErrorSec - runTm);
			}
			restartCount++;
			time(&restartTime);
			//restart service
			k = fafio.doFork();
	    }
	}
	DbgLog::setThreadName("main",false);
	takeOwnership();
	buffer->indaemon = true;
}

void ProgInstance::restartDaemon() {
	exit(251);
}

///Initializes variable that hold's program start time.
/** You need to call this function when you plan to use function
 * getUpTime without initializing the ProgInstance object and entering
 * to daemon mode. Function performs initialization only for first calling,
 * any other calls does nothing.
 */
void ProgInstance::initializeStartTime() {
	if (startTime == 0) time(&startTime);
	if (restartTime < startTime) restartTime = startTime;
}

///gets total CPU time consumed by this process
/**
 * @return total CPU time in milliseconds. For multicore CPUs, returns sums through all cores
 */
natural ProgInstance::getCPUTime() {
	if (CLOCKS_PER_SEC < 1000) {
		return (clock() * 1000)/CLOCKS_PER_SEC;
	} else {
		return (clock() / (CLOCKS_PER_SEC/1000));
	}
}


///gets process memory usage
/**
 * @return returns net memory usage. It excludes memory allocated by the allocator
 * itself and memory unavailable due memory fragmentation.
 */
natural ProgInstance::getMemoryUsage() {
	struct mallinfo res = mallinfo();
	return res.uordblks;
}


///True, if instance can be in standard stage
/** There is no platform, which will have this false */
const bool ProgInstance::hasStage_standard = true;
///True, if instance can be in predaemon stage
/** Some platforms (Windows) need this stage to enter daemon stage.
 *  You can
 */
const bool ProgInstance::hasStage_predaemon = false;
///True, if instance can enter to daemon stage
/** If there is false, platform cannot create services running
 * on background.
 */
const bool ProgInstance::hasStage_daemon = true;
///True, if instance can be controller
/** If there is false, controller will unable to detect, whether
 * daemon is running and will unable to send messages to it.
 */
const bool ProgInstance::hasStage_controller = true;

}



