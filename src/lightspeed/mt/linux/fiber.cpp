/*
 * fiber.cpp
 *
 *  Created on: 8.10.2010
 *      Author: ondra
 */

#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../fiber.h"
#include "../../base/exceptions/errorMessageException.h"
#include "../../base/sync/threadVar.h"
#include "../exceptions/fiberException.h"
#include "../../base/containers/constStr.h"
#include "../../base/exceptions/stdexception.h"


namespace LightSpeed {



	//default stack has 512KB
	static const natural defaultStackSize = 65536*sizeof(natural);


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
	class FiberContext: public ucontext_t {
	public:
		//true to force fiber stop on wakeup
		bool stopSignal;
		//true to destroy fiber on other fiber wakeup
		bool termSignal;
		//reason for wakeUp
		natural wakeUpReason;
		//space reserved for stack (continues outside of object)
		natural stack[1];

		static void fiberBootstrap(const FiberFunction *fn) throw();
		static void exitFiber();
	};

	Fiber::Fiber()
		:ctx(0),caller(0),master(0) {}

	//special exception - cannot be caught by std::exception - to force fiber stop
	enum StopFibeException {stopFiberException};

	Fiber::~Fiber() {
		try {
			//master thread cannot exited
			if (isMaster())
				//free context
				free(ctx);
			else {
				//stop fiber, when it running
				while (checkState()) {
					//stop should throw special exception
					stop();
				}
			}
		} catch (...) {
			if (!std::uncaught_exception()) throw;
		}
	}

	//bootstrap procedure - called to start the fiber
	void FiberContext::fiberBootstrap(const FiberFunction *fn) throw() {


		natural sz = fn->ifc().getObjectSize();
		void *buff = alloca(sz);
		AllocInBuffer abuff(buff,sz);
		AllocPointer<FiberFunction::Ifc> fnptr(fn->clone(abuff));

		try {

			(*fnptr)();


		} catch (Fiber::StopFiberException &e) {

		} catch (const Exception &e) {
			Fiber::current().exitException = e.clone();
		} catch (std::exception &e) {
			Fiber::current().exitException = StdException(THISLOCATION,e).clone();
		} catch (...) {
			Fiber::current().exitException = UnknownException(THISLOCATION).clone();
		}
		//request terminate
		Fiber::current().ctx->termSignal = true;

	}

	class FiberCleanup: public IThreadVarDelete {
	public:
		void operator()(void *ptr) {
			Fiber *f = reinterpret_cast<Fiber *>(ptr);
			if (f->isMaster()) {
				delete f;
			}
		}
	};


	void Fiber::start(const IFiberFunction &fn, natural stackSize ) {
		// handle default stack size
		if (stackSize == 0) stackSize = defaultStackSize;
		else stackSize = sizeof(natural) * stackSize;

		ITLSTable &tbl = ITLSTable::getInstance();
		Fiber *fb = currentFiber[tbl];
		if (fb == 0) fb = &createMasterFiber();

		//retrieve current fiber
		Fiber &cur = *fb;

		//create context for fiber
		FiberContext *fctx = (FiberContext *)malloc(
									stackSize+sizeof(FiberContext));
		if (fctx == 0)
			throw OutOfMemoryException(THISLOCATION);

		//reset context
		memset(fctx,0,sizeof(FiberContext));

		//retrieve current context as initialization of new context
		if (getcontext(fctx) == -1) {
			int err = errno;
			free(fctx);
			throw FiberErrorException(THISLOCATION,err);
		}

		//set new stack
		fctx->uc_stack.ss_sp = fctx->stack;
		fctx->uc_stack.ss_size = stackSize;
		//define link to master context
		fctx->uc_link = cur.master->ctx;

		void (*bsfn)(void) = (void (*)(void))&FiberContext::fiberBootstrap;
		//create new context
		makecontext(fctx,bsfn,1,&fn);

		//set context to the fiber instance
		this->ctx = fctx;
		//set current master fiber
		this->master = cur.master;
		//set caller - fiber which started this fiber
		this->caller = fb;

		currentFiber.set(tbl,this);

		switchFibers(fb,this);
	}

	void Fiber::stop() {
		//silently leave stopped fiber stopped
		if (ctx == 0) return;
		//signal fiber to stop
		ctx->stopSignal = true;
		//wake fiber;
		wakeUp();
	}

	void Fiber::wakeUp(natural reason) throw() {
		//retrieve current fiber
		Fiber &cur = current();
		//if resume to current fiber - do nothing
		if (&cur == this || ctx == 0) return;
		//store caller
		caller = &cur;
		//set reason
		ctx->wakeUpReason = reason;
		//make fiber active
		switchFibers(&cur,this);
	}

	Fiber* Fiber::currentPtr() {
		ITLSTable &tbl = ITLSTable::getInstance();
		Fiber *cur = currentFiber[tbl];
		return cur;
	}

	void Fiber::switchFibers(Fiber *from, Fiber *to) {

		if (from == to) return;
		//can't switch fibers while exception
		if (std::uncaught_exception()) return;

//		ITLSTable &tbl = ITLSTable::getInstance();
		//set new current fiber (to)
//		currentFiber.set(tbl,to);
		//swap context
		if (swapcontext(from->ctx,to->ctx) == -1) {
			//report error
			throw FiberErrorException(THISLOCATION,errno);
		}

		ITLSTable &tbl = ITLSTable::getInstance();
		//receive previous fiber
		Fiber *last = currentFiber[tbl];
		//retrieve context of previous fiber
		FiberContext *lastCtx = last->ctx;
		//prev fiber has been terminated
		if (lastCtx->termSignal) {
			//free fiber context and stack
			free(lastCtx);
			//set not active state
			last->ctx = 0;
		}
		//update current fiber
		currentFiber.set(tbl,from);
		//send stop exception to exit all scopes
		if (from->ctx->stopSignal)
			throw stopFiberException;
	}

	bool Fiber::checkState() const {
		if (exitException != nil && current().isMaster() && !std::uncaught_exception()) {
			PException x = exitException;
			exitException = nil;
			x->throwAgain(THISLOCATION);
		}
		return (ctx != 0);
	}

	bool Fiber::isMaster() const{
		return master == this;
	}

	Fiber &Fiber::current() {
		ITLSTable &tbl = ITLSTable::getInstance();
		Fiber *cur = currentFiber[tbl];
		if (cur) return *cur;
		else throw NoCurrentFiberException(THISLOCATION);
	}

	natural Fiber::sleep() {
		Fiber &cur = current();
		if (cur.caller == 0 || cur.caller->ctx == 0) {
			if (cur.master == 0 || cur.master->ctx == 0)
				throw FiberNotRunningException(THISLOCATION,0);
			switchFibers(&cur,cur.master);
		} else {
			switchFibers(&cur,cur.caller);
		}
		return cur.ctx->wakeUpReason;
	}

	static FiberCleanup fiberCleanup;

	Fiber &Fiber::createMasterFiber() {
		ITLSTable &tbl = ITLSTable::getInstance();
		Fiber *cur = masterFiber[tbl];
		if (cur) {
			return *cur->master;
		} else {
			cur = new Fiber();
			cur->ctx = (FiberContext *)malloc(sizeof(FiberContext));
			currentFiber.set(tbl,cur);
			masterFiber.set(tbl,cur,&fiberCleanup);
			cur->master = cur;
			return *cur;
		}
	}

	bool Fiber::destroyMasterFiber() {
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
		return false;
	}

	Fiber *Fiber::getCaller() {
		return caller;
	}

	Fiber *Fiber::getMaster() {
		return master;
	}





}


