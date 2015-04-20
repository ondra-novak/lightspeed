/*
 * fthread.cpp
 *
 *  Created on: 22.8.2011
 *      Author: ondra
 */

#include <unistd.h>
#include <semaphore.h>
#include "../../base/memory/inBlockAlloc.h"
#include "../../base/exceptions/stdexception.h"
#include "../atomic.h"
#include "../../base/countof.h"
#include "../../base/memory/singleton.h"
#include "../../base/containers/autoArray.tcc"
#include "../thread.h"

#include "../../base/debug/dbglog.h"
#include "../../base/sync/threadVar.h"
#include "../notifier.h"
#include "../exceptions/threadException.h"
#include "../../base/framework/app.h"
#include "../../base/linux/seh.h"
#include "signalException.h"
#include "../threadHook.h"

namespace LightSpeed {

static const natural flagAttached = 2;

struct ThreadBootstrap {

	const IThreadFunction &fn;
	Notifier unblockCaller;
	Thread *owner;

	ThreadBootstrap(const IThreadFunction &fn,Thread *owner):fn(fn),owner(owner) {}

};

static int enabledSignals[] = {SIGSEGV, SIGILL,SIGABRT,SIGSTOP,SIGCONT};
static int enabledSignalsForMaster[] = {SIGTERM,SIGKILL, SIGSEGV, SIGILL,SIGINT,SIGABRT,SIGSTOP,SIGCONT};


class ThreadContext;

__thread ThreadContext * currentContext = 0;
static inline ThreadContext *getCurrentContext() {
	return currentContext;
}

static inline void setCurrentContext(ThreadContext *ctx) {
	currentContext = ctx;
}



// Thread state - stored in thread stack
class ThreadContext: public DynObject {
public:

	//owner Thread object
	Thread *owner;
	//pointer to current TLS table
	ITLSTable *tlstable;


	ThreadContext() {
	}


	virtual ~ThreadContext() {
	}

	virtual void handleCleanup();

	static void *bootstrap(void *data);


	void handleException(const Exception &e) {
		IApp *app = IApp::currentPtr();
		if (app != 0) {
			//report exception to the application
			app->onThreadException(e);
		}
		finishThread();
	}
	void finishThread() {
		//open gate, when thread finished
		if (owner) {
			//reset context pointer - it is no longer available
			writeReleasePtr<ThreadContext>(&owner->threadContext,0);
			owner->getJoinObject().open();
		}
	}
protected:
	ThreadContext(const ThreadContext &other);

};


static void adjustThreadSignals(bool master) {

	sigset_t sigMask;
	//modify linux signals for threading

	sigfillset(&sigMask);
	if (master) {
		for (natural i = 0; i < countof(enabledSignalsForMaster); i++)
			sigdelset(&sigMask,enabledSignalsForMaster[i]);
	} else {
		for (natural i = 0; i < countof(enabledSignals); i++)
			sigdelset(&sigMask,enabledSignals[i]);
	}

	pthread_sigmask(SIG_BLOCK,&sigMask,0);

	LinuxSEH::init();

}

static bool threadSignalHandler(int sig, siginfo_t *, void *) {
	ConstStrA threadName = DbgLog::getThreadName();
	fprintf (stderr,"Uncaugh signal %d in thread: %s \n", sig, threadName.data());
	LinuxSEH::output_stack_trace();
	return true;
}

void *ThreadContext::bootstrap(void * data) {

	adjustThreadSignals(false);
	//retrieve bootstrap data
	ThreadBootstrap *bs = (ThreadBootstrap *)data;

	//calculate size of context + pointer to allocator
	natural bksize = DynObject::SizeOfObject<ThreadContext>::size;
	//allocate space at stack
	void *bk = alloca(bksize);
	//create allocator
	AllocInBuffer ctxbf(bk,bksize);
	//allocate context
	AllocPointer<ThreadContext> ctx(new (ctxbf) ThreadContext);


	//calculate size of function + pointer to allocator
	bksize = bs->fn.getObjectSize() + sizeof(void *);
	//create space at stack
	bk = alloca(bksize);
	//create allocator
	AllocInBuffer fnbf(bk,bksize);
	//allocate function
	Message<void> fn(bs->fn.clone(fnbf));

	//store owner from bootstrap
	ctx->owner = bs->owner;

	//receive current state of TLS table
	//calculate space needed
	bksize = (TLSAllocator::getInstance().getMaxIndex() + 1) * sizeof (TLSRecord);
	//reserve space at stack
	bk = alloca(bksize);
	//create allocator for AutoArray
	InBlockAlloc bkalloc(bk,bksize);
	//create TLS table for this thread
	TLSTable<InBlockAlloc> tlstable(bkalloc);
	//refer TLS table in the context
	ctx->tlstable = &tlstable;


	//store thread ID
	ctx->owner->id = pthread_self();
	//reset finish flag
	ctx->owner->flags = 0;
	//set thread context
	ctx->owner->threadContext = ctx;

	//set pointer to currentContext global TLS variable
	setCurrentContext(ctx);

	__seh_try_fn(threadSignalHandler) {


		//start thread
		try {
			AbstractThreadHook::callOnThreadInitHooks(*ctx->owner);

			ctx->owner->getJoinObject().close();
			//unblock caller - bootstrap data vanished
			bs->unblockCaller.wakeUp();
			//wrap function call to catch exceptions by thread hooks
			try {
				//call function
				fn.deliver();

			} catch (...) {
				AbstractThreadHook::callOnThreadExceptionHooks(*ctx->owner);
				throw;
			}

			AbstractThreadHook::callOnThreadExitHooks(*ctx->owner);

			ctx->finishThread();

		} catch (const Exception &e) {
			//on LightSpeed::Exception - handle it
			ctx->handleException(e);
		} catch (const std::exception &e) {
			//on std::exception
			StdException stde(THISLOCATION,e);
			//handle it
			ctx->handleException(stde);
		} catch (...) {
			//on unknown exception
			UnknownException e(THISLOCATION);
			//handle it
			ctx->handleException(e);
		}
		//reset pointer to current context - preventing any possible future accessing destroyed structure
		setCurrentContext(0);
		//note: context is not reset, because owner could vanish now.
		return 0;
	} __seh_except(s) {
		UnexpectedSignalException err(THISLOCATION,s);
		AppBase::current().onThreadException(err);
		//reset pointer to current context - preventing any possible future accessing destroyed structure
		setCurrentContext(0);
		std::terminate();
		return 0;
	}

	//should never reach
	return 0;
}

void ThreadContext::handleCleanup()
{
	/* Method called when ordinary thread is destroying itself.
	Context of the thread is allocated at stack, so it cannot be
	deleted. In this situation, Thread object can be destroyed
	and thread becomes without owner.

	Thread can be later reattached

	*/
	if (owner) {
		finishThread();
		owner = 0;
	}
	/* join object will be opened, because thread is officially terminated */
}


Thread::Thread():threadContext(0),joinObject(Gate::stateOpen),flags(0),id(nil) {

}

Thread::Thread(const IThreadFunction &fn):threadContext(0),joinObject(Gate::stateOpen),flags(0),id(nil)  {
	start(fn);
}

Thread::Thread(const IThreadFunction &fn, natural stackSize):threadContext(0),joinObject(Gate::stateOpen),flags(0),id(nil)  {
	start(fn,stackSize);
}




Thread::~Thread() try {
	/*if current thread terminates itself
	  this is not good practice, but may happen
	  and also this is only way how to destroy attached
	  Thread */
	if (getCurrentContext() == threadContext) {
		/* we know, that current context will not disappear */
		/* perform cleanup */
		if (threadContext) threadContext->handleCleanup();
	} else {
		/* called from different thread*/

		stop();

	}
} catch (...) {
	if (std::uncaught_exception()) return;
}

void Thread::start(const IThreadFunction &fn) {
	start(fn,0);
}

void Thread::start(const IThreadFunction &fn, natural stackSize) {
	/* This sequence creates fake context
	  to allow MT access to the start function.
	  Trying to start already started thread causes exception.
	  But until thread is not constructed, we need to make object to look likes that thread is already running
	  So let's create fake thread context, which can be used to ask basic values.
	  This allows functions isRunning and join() to work correctly.
	  */

	ThreadContext fakeContext;
	//redirect TLS to current thread tls.
	/* This is hack, because TLS there table is changing during thread existence. But note, that
	 * it is not good practice to access TLS directly and is better to use synchronization. So you
	 * will deal with fully constructred thread
	 */
	fakeContext.tlstable = &ITLSTable::getInstance();
	fakeContext.owner = this;

	///Now, try to set new context. If there is one, function fails
	if (lockCompareExchangePtr<ThreadContext>(&threadContext,0,&fakeContext) != 0)
		throw ThreadBusyException(THISLOCATION);
	//close join object - now other threads can see, that thread is already running
	joinObject.close();

	//uff - starter thread is alone, we can perform MT unsafe tasks.
	//acquire master thread - ensure, that MT environment is already up.
	getMaster();

	//initialize sleeper
	if (sleeper == nil) sleeper = DefaultConstructor<ThreadSleeper>();

	//create bootstrap informations
	ThreadBootstrap bootstrap(fn,this);

	//setup thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//every thread is detached, because LightSpeed don't need to join threads explicitly and joining is implemented by LLEvent
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	if (stackSize >= 4096) pthread_attr_setstacksize(&attr,stackSize);

	pthread_t id = 0;
	try {

		//try to create thread
		int e = pthread_create(&id,&attr,&ThreadContext::bootstrap,&bootstrap);
		if (e) {
			throw UnableToStartThreadException(THISLOCATION,e);
		}

		//now block the caller until new thread is not fully up to remove bootstrap informations
		bootstrap.unblockCaller.wait(nil);

	} catch (...) {
		pthread_attr_destroy(&attr);
		//thread creation failed. We must remove fake context
		threadContext = 0;
		//continue exception
		throw;
	}

	pthread_attr_destroy(&attr);

}

void Thread::join() {
	getJoinObject().wait(nil);
	threadContext = 0;
}

bool Thread::isRunning() const {
	return !joinObject.isOpened();
}

///Retrieves current thread
Thread &Thread::current() {
	Thread *p = currentPtr();
	if (p == 0) {
		if (!Thread::isThreaded()) return Thread::getMaster();
		else throw NoCurrentThreadException(THISLOCATION,pthread_self());
	}
	return *p;
}

///Retrieves pointer to current thread
Thread *Thread::currentPtr() {
	ThreadContext *ctx = getCurrentContext();
	return ctx?ctx->owner:0;
}

bool Thread::impSleep(const Timeout &tm, natural &reason) {
	bool r = sleeper->sleep(tm);
	if (r) reason=sleeper->getReason();
	return r;
}

void Thread::deepSleep(const Timeout &timeout) {
	while (!sleep(timeout)) {}
}

void Thread::wakeUp(natural reason) throw() {
	if (sleeper != nil) sleeper->wakeUp(reason);
}

///Retrieves ID of thread
ThreadId Thread::getThreadId() const {
	return id;
}

///signals thread to finish
void Thread::finish() {;
	flags |= flagFinish;
	wakeUp(naturalNull);
}

///true, if thread should finish
bool Thread::canFinish() {
	return (current().flags & flagFinish) != 0;
}

///true if thread is finishing already
bool Thread::isFinishing() const {
	return (current().flags & flagFinish) != 0;
}

///retrieves reference to TLS table
ITLSTable &Thread::getTls() {
	if (isRunning()) return *threadContext->tlstable;
	throw ThreadNotRunningException(THISLOCATION);
}

///retrieves reference to TLS table
const ITLSTable &Thread::getTls() const {
	if (isRunning()) return *threadContext->tlstable;
	throw ThreadNotRunningException(THISLOCATION);
}

///Retrieves reference to Gate object
/** Gate object becomes signaled when thread finishes */
Gate &Thread::getJoinObject() {
	return joinObject;
}




///MT TLS allocator
/**
 * Allocates in TLS table, guards TLS table by lock
 */
class MTTLSAllocator: public ITLSAllocator {
public:

    virtual natural allocIndex() {
    	Synchronized<FastLock> _(lock);
    	return allocator.allocIndex();
    }
    virtual void freeIndex(natural index) {
    	Synchronized<FastLock> _(lock);
    	allocator.freeIndex(index);

    }
    virtual natural getMaxIndex() const {
    	return allocator.getMaxIndex();
    }

    MTTLSAllocator():allocator(ITLSAllocator::getInstance()) {}

protected:
    FastLock lock;
	ITLSAllocator &allocator;

};

class ThreadAttachContext: public ThreadContext {
public:

	ThreadAttachContext(bool keepContext);
	void handleCleanup();
	void unkeep();
	ThreadAttachContext *nextContext;
protected:
	bool keepContext;
	TLSTable<> tls;
};

ThreadAttachContext::ThreadAttachContext( bool keepContext )
	:keepContext(keepContext)
{
	this->tlstable = &tls;
	this->owner = 0;
}

void ThreadAttachContext::handleCleanup()
{
	if (owner) {
		AbstractThreadHook::callOnThreadExitHooks(*owner);
		finishThread();
		owner = 0;
	}
	if (!keepContext) {
		setCurrentContext(0);
		delete this;
	}
}

void ThreadAttachContext::unkeep()
{
	if (owner) keepContext = false;
	else delete this;
}

///Master thread object - created on first call of the function Thread::getMaster()
class MasterThread: public Thread, public RefCntObj {
public:

	///Retrieves master-thread's TLS allocator
	ITLSAllocator &getTLSAllocator()  {return tlsalloc;}

	///Constructs master thread
	MasterThread() {
		threadContext = &context;

		if (sleeper == nil) sleeper = DefaultConstructor<ThreadSleeper>();
		id = pthread_self();

		adjustThreadSignals(true);
		//store reference to master thread
		context.owner = this;
		//retrieve global TLS table - use as master thread's TLS table
		context.tlstable = &ITLSTable::getInstance();

		//store pointer to TLS-Table instance retriever
		oldTableInstance = ITLSTable::getTLSFunction();
		//store pointer to TLS-Allocator object
		oldAllocInstance = ITLSAllocator::getTLSFunction();


		joinObject.close();
		flags = 0;

		//redirect allocator
		ITLSAllocator::setTLSFunction(&getTLSAlloc);
		//redirect table
		ITLSTable::setTLSFunction(&getTLS);

		keptContext = 0;




		//mark down that application is threaded now
		threaded = true;

		setCurrentContext(&context);

		AbstractThreadHook::callOnThreadInitHooks(*this);
	}

	void updatePointers() {
		//redirect allocator
		ITLSAllocator::setTLSFunction(&getTLSAlloc);
		//redirect table
		ITLSTable::setTLSFunction(&getTLS);
	}

	///Destructor - should be called at exit only
	virtual ~MasterThread() {
		removeAllContexts();
		AbstractThreadHook::callOnThreadExitHooks(*this);
		//restore old pointers
		ITLSAllocator::setTLSFunction(oldAllocInstance);
		ITLSTable::setTLSFunction(oldTableInstance);
		//mark down, that application is not threaded
		threaded = false;
		setCurrentContext(0);
		threadContext = 0;
	}

	///Retrieves threaded TLS allocator
	static ITLSAllocator &getTLSAlloc();

	///Retrieves threaded TLS table
	static ITLSTable &getTLS();

	///true, if application is threaded
	static bool threaded;


	void registerContext(ThreadAttachContext *ctx);

	void cleanupContext();

	void removeAllContexts();

protected:

	MTTLSAllocator tlsalloc;
	ThreadContext context;
	ITLSAllocator::fn_GetTLSAllocator oldAllocInstance;
	ITLSTable::fn_GetTLS oldTableInstance;
	ThreadAttachContext *keptContext;


};

void MasterThread::registerContext( ThreadAttachContext *ctx )
{
	cleanupContext();
	ctx->nextContext = keptContext;
	while (lockCompareExchangePtr<ThreadAttachContext>(&keptContext,ctx->nextContext,ctx) != ctx->nextContext) {
		ctx->nextContext = keptContext;
	}
}

void MasterThread::cleanupContext()
{
	ThreadAttachContext *save = keptContext;
	while (lockCompareExchangePtr<ThreadAttachContext>(&keptContext,save,0) != keptContext)
		save = keptContext;
	while (save) {
		ThreadAttachContext *subj = save;
		save = save->nextContext;
		if (save->owner == 0) {
			delete subj;
		} else {
			subj->nextContext = keptContext;
			while (lockCompareExchangePtr<ThreadAttachContext>(&keptContext,subj->nextContext,subj) != subj->nextContext) {
				subj->nextContext = keptContext;
			}
		}
	}
}

void MasterThread::removeAllContexts()
{
	ThreadAttachContext *save = keptContext;
	while (lockCompareExchangePtr<ThreadAttachContext>(&keptContext,save,0) != save)
		save = keptContext;
	while (save) {
		ThreadAttachContext *subj = save;
		save = save->nextContext;
		subj->unkeep();
	}
}


bool MasterThread::threaded = false;
static RefCntPtr<MasterThread> master = 0;

///Retrieves master thread
Thread &Thread::getMaster() {
	if (master == nil) {
		master = new MasterThread();
	}
	return *master;
}



bool Thread::isThreaded() {
	return master != nil;
}




bool Thread::attach( bool keepContext, IRuntimeAlloc *contextAlloc /*= 0*/ )
{
	ThreadContext *ctx = getCurrentContext();
	if (ctx) {
		if (ctx->owner) return false;
		ctx->owner = this;
		threadContext = ctx;
		flags = flagAttached;
		joinObject.close();
		AbstractThreadHook::callOnThreadInitHooks(*this);
		return true;
	}


	if (contextAlloc)
		ctx = new(*contextAlloc) ThreadAttachContext(keepContext);
	else
		ctx = new ThreadAttachContext(keepContext);

	ctx->owner = this;
	threadContext = ctx;
	flags = flagAttached;
	if (sleeper == nil) sleeper = DefaultConstructor<ThreadSleeper>();
	id = pthread_self();


	if (keepContext) {
		if (master == nil) Thread::getMaster();
		master->registerContext(static_cast<ThreadAttachContext *>(ctx));
	}
	setCurrentContext(ctx);
	joinObject.close();
	AbstractThreadHook::callOnThreadInitHooks(*this);
	return true;
}

//not necesery on linux
struct LibInfo {
	natural versionId;

};

//not necesery on linux
ITLSAllocator & MasterThread::getTLSAlloc()
{
	if (master != nil) return master->tlsalloc;
	else return _stGetTLSAllocator();
}

ITLSTable & MasterThread::getTLS()
{
	if (master != nil) {
		ThreadContext *ctx = getCurrentContext();
		if (ctx) return *ctx->tlstable;
		else throw NoCurrentThreadException(THISLOCATION,pthread_self());
	}
	else return _stGetTLS();
}

const char *linuxThreads_signalExceptionMsg = "Unexpected signal caught: %1";

}


