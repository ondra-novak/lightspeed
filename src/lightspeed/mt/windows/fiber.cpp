#include "../fiber.h"
#include "../../base/sync/threadVar.h"
#include "../../base/exceptions/stdexception.h"
#include "../exceptions/fiberException.h"

namespace LightSpeed {

	//Contains address of active fiber in current thread
	static ThreadVar<Fiber> currentFiber;
	//Contains address of master fiber in current thread
	static ThreadVar<Fiber> masterFiber;

	//context for the fiber
	/*
	* Context is allocated at heap as single block
	* it contains ucontext_t descriptor, pointer to
	* associated object, and space for stack
	*
	*/
	class FiberContext {
	public:
		//true to force fiber stop on wakeup
		bool stopSignal;
		//true to destroy fiber on other fiber wakeup
		bool termSignal;
		//reason for wakeUp
		natural wakeUpReason;

		void *fiberID;

		static void CALLBACK fiberBootstrap(_In_ PVOID lpParameter) throw();
		static void exitFiber();
	};

	Fiber::Fiber()
		:ctx(0), caller(0), master(0) {}

	//special exception - cannot be caught by std::exception - to force fiber stop
	enum StopFibeException { stopFiberException };


	Fiber::~Fiber()
	{
		try {
			void *fibid = ctx->fiberID;
			//free context
			delete ctx;

			//master thread cannot exited
			if (isMaster()) {

				ConvertFiberToThread();
			} 
			else {
				//stop fiber, when it running
				while (checkState()) {
					//stop should throw special exception
					stop();
				}
			}
		}
		catch (...) {
//TODO handle exceptions			if (!std::uncaught_exception()) throw;
		}

	}

	//bootstrap procedure - called to start the fiber
	void FiberContext::fiberBootstrap(_In_ PVOID lpParameter) throw() {


		const FiberFunction *fn = (const FiberFunction *)lpParameter;


		natural sz = fn->ifc().getObjectSize();
		void *buff = alloca(sz);
		AllocInBuffer abuff(buff, sz);
		AllocPointer<FiberFunction::Ifc> fnptr(fn->clone(abuff));

		try {

			(*fnptr)();


		}
		catch (Fiber::StopFiberException &) {

		}
		catch (const Exception &e) {
			Fiber::current().exitException = e.clone();
		}
		catch (std::exception &e) {
			Fiber::current().exitException = StdException(THISLOCATION, e).clone();
		}
		catch (...) {
			Fiber::current().exitException = UnknownException(THISLOCATION).clone();
		}
		//request terminate
		Fiber::current().ctx->termSignal = true;

		//function never return
		Fiber::sleep();

		abort();

	}

	static void fiberCleanup(void *ptr) {
		//called by ThreadVar delete handler
		Fiber *f = reinterpret_cast<Fiber *>(ptr);
		//should be master fiber
		if (f->isMaster()) {
			//delete it now
			delete f;
		}
	}


	void Fiber::start(const IFiberFunction &fn, natural stackSize /*= 0*/)
	{
		//check whether fiber is already running
		if (ctx) throw FiberErrorException(THISLOCATION, ERROR_ALREADY_EXISTS);
		// receive current fiber
		ITLSTable &tbl = ITLSTable::getInstance();
		Fiber *volatile fb = currentFiber[tbl];
		//there should be always fiber, even master fiber. 
		//If no fiber found, make current thread as master fiber
		if (fb == 0) fb = &createMasterFiber();

		//store current fiber
		Fiber &cur = *fb;

		//create fiber's context
		FiberContext *fctx = new FiberContext;
		fctx->stopSignal = false;
		fctx->termSignal = false;
		fctx->wakeUpReason = 0;		

		//set fiber's function 
		LPFIBER_START_ROUTINE bsfn = &FiberContext::fiberBootstrap;
		//create fiber instance and set fiber's ID to context
		fctx->fiberID = CreateFiberEx(stackSize, 0, FIBER_FLAG_FLOAT_SWITCH, bsfn, (LPVOID)&fn);

		//set context to the fiber instance
		this->ctx = fctx;
		//set current master fiber
		this->master = cur.master;
		//set caller - fiber which started this fiber
		this->caller = fb;

		//switch to fiber - because initial schedule is required now
		switchFibers(fb, this);

	}

	void Fiber::stop()
	{
		//silently leave stopped fiber stopped
		if (ctx == 0) return;
		//signal fiber to stop
		ctx->stopSignal = true;
		//wake fiber;
		wakeUp();

	}

	void Fiber::wakeUp(natural reason /*= 0*/) throw()
	{
		//retrieve current fiber
		Fiber &cur = current();
		//if resume to current fiber - do nothing
		if (&cur == this || ctx == 0) return;
		//store caller
		caller = &cur;
		//set reason
		ctx->wakeUpReason = reason;
		//make fiber active
		switchFibers(&cur, this);

	}

	void Fiber::switchFibers(Fiber *from, Fiber *to) {
		
		if (from == to) return;
		//can't switch fibers while exception
		if (std::uncaught_exception()) return;

		SwitchToFiber(to->ctx->fiberID);

		ITLSTable &tbl = ITLSTable::getInstance();
		//receive previous fiber
		//NOTE last != from. In case that from->to->other->from => last=other		
		Fiber *last = currentFiber[tbl];
		//retrieve context of previous fiber
		FiberContext *lastCtx = last->ctx;
		bool throwStop = false;

		//prev fiber has been terminated
		if (lastCtx->termSignal) {
			//free fiber context and stack
			delete lastCtx;
			//set not active state
			last->ctx = 0;
		}
		else if (from->ctx->stopSignal) {
			throwStop = true;
		} 
		//update current fiber - because we returned back to "from", set from as current
		currentFiber.set(tbl, from);
		//send stop exception to exit all scopes
		if (throwStop)
			throw stopFiberException;
	}


	bool Fiber::checkState() const
	{
		if (exitException != nil && current().isMaster() && !std::uncaught_exception()) {
			PException x = exitException;
			exitException = nil;
			x->throwAgain(THISLOCATION);
		}
		return (ctx != 0);
	}

	bool Fiber::isMaster() const
	{
		return master == this;
	}

	Fiber & Fiber::current()
	{
		ITLSTable &tbl = ITLSTable::getInstance();
		Fiber *cur = currentFiber[tbl];
		if (cur) return *cur;
		else throw NoCurrentFiberException(THISLOCATION);
	}

	Fiber * Fiber::currentPtr()
	{
		ITLSTable &tbl = ITLSTable::getInstance();
		Fiber *cur = currentFiber[tbl];
		return cur;
	}

	LightSpeed::natural Fiber::sleep()
	{
		Fiber &cur = current();
		if (cur.caller == 0 || cur.caller->ctx == 0) {
			if (cur.master == 0 || cur.master->ctx == 0)
				throw FiberNotRunningException(THISLOCATION, 0);
			switchFibers(&cur, cur.master);
		}
		else {
			switchFibers(&cur, cur.caller);
		}
		return cur.ctx->wakeUpReason;
	}

	Fiber * Fiber::getCaller()
	{
		return caller;
	}

	Fiber * Fiber::getMaster()
	{
		return master;
	}

	Fiber & Fiber::createMasterFiber()
	{
		ITLSTable &tbl = ITLSTable::getInstance();
		Fiber *cur = masterFiber[tbl];
		if (cur) {
			return *cur->master;
		}
		else {
			ConvertThreadToFiberEx(0, FIBER_FLAG_FLOAT_SWITCH);
			cur = new Fiber();
			cur->ctx = new FiberContext;
			cur->ctx->stopSignal = false;
			cur->ctx->termSignal = false;
			cur->ctx->wakeUpReason = 0;

			currentFiber.set(tbl, cur);
			masterFiber.set(tbl, cur, &fiberCleanup);

			cur->master = cur;
			return *cur;
		}

	}

	bool Fiber::destroyMasterFiber()
	{
		ITLSTable &tbl = ITLSTable::getInstance();
		Fiber *cur = currentFiber[tbl];
		Fiber *mas = masterFiber[tbl];
		if (mas == 0) {
			return true;
		}
		if (cur != mas) {
			return false;
		}
		currentFiber.unset(tbl);
		return true;
	}

}