#pragma once

#include "promise.h"


#include "../containers/deque.tcc"
#include "../containers/autoArray.tcc"
#include "../exceptions/canceledException.h"
#include "../exceptions/pointerException.h"

namespace LightSpeed {

template<typename T>
bool Future<T>::Value::resolved() const throw()
{
	return isResolved();
}

template<typename T>
bool Future<T>::Value::isResolved() const throw()
{
	return (value != nil || exception != nil);
}

template<typename T>
IPromiseControl::State Future<T>::Value::getState() const throw()
{
	Synchronized<FastLock> _(lock);
	if (resolving) return stateResolving;
	return isResolved() ? stateResolved : stateNotResolved;
}

template<typename T>
void Future<T>::Value::resolve( const T &result ) throw()
{
	resolveInternal<Optional<T>, T>(value, result);
}

template<typename T>
void Future<T>::Value::resolve( const IConstructor<T> &result ) throw ()
{
	resolveInternal<Optional<T>, IConstructor<T> >(value, result);
}

template<typename T>
void Future<T>::Value::resolve(const PException &e ) throw ()
{
	resolveInternal<PException, PException>(exception, e);
}

template<typename T>
template<typename X, typename Y>
void Future<T>::Value::resolveInternal(X &var, const Y & result)
{
	//temporary array of loaded observers
	Observers cpy;
	//lock the promise internals
	Synchronized<FastLock> _(lock);
	//if already resolved, prevent future resolutions
	if (isResolved()) return;
	//set to resolving state
	resolving = true;
	//store value
	var = result;
	//cycle until the list of observers is empty
	while (!observers.empty()) {
		IObserver *x = observers.getFront();
		observers.popFront();
		SyncReleased<FastLock> _(lock);
		x->resolve(var);
	}
	//reset state
	resolving = false;
}


template<typename T>
void Future<T>::Value::registerObserver( IObserver *ifc )
{
	//lock promise internals
	lock.lock();
	//if promise is not currently resolving
	if (!resolving) {
		//if there is value
		if (value != nil) {
			lock.unlock();
			//call resolve
			ifc->resolve(value);
			return;
		}
		//if there is exception
		if (exception != nil) {
			lock.unlock();
			//reject 
			ifc->resolve(exception);
			return;
		}
	}
	//otherwise add the observer to the list
	observers.pushBack(ifc);
	//unlock internals
	lock.unlock();
}

template<typename T>
bool Future<T>::Value::unregisterObserver( IObserver *ifc )
{
	bool found = false;
	Synchronized<FastLock> _(lock);
	natural cnt = observers.length();
	natural i = 0;
	while (i < cnt) {
		IObserver *x = observers.getBack();
		observers.popBack();
		if (x == ifc) {
			if (i < cnt / 2) {
				while (i > 0) {
					x = observers.getFront();
					observers.popFront();
					observers.pushBack(x);
					i--;
				}
				return true;
			}
			else {
				found = true;
			}
		}
		else {
			observers.pushFront(x);
		}
		i++;
	}
	return found;
}


template<typename T>
void Future<T>::addObserver( IObserver*ifc )
{
	if (future == nil) {
		init();
	}
	future->registerObserver(ifc);
}

template<typename T>
bool Future<T>::removeObserver( IObserver *ifc )
{
	if (future == nil) {
		return false;
	}
	return future->unregisterObserver(ifc);
}

template<typename T>
const T & Future<T>::wait( const Timeout &tm /*= Timeout()*/ ) const
{
	class Ntf: public IObserver {
	public:
		ISleepingObject *thread;
		const T *result;
		PException exception;
		
		Ntf(ISleepingObject *thread):thread(thread),result(0),exception(0) {}

		virtual void resolve( const T &result ) throw()
		{
			this->result = &result;
			thread->wakeUp();
		}

		virtual void resolve(const PException &e ) throw()
		{
			this->exception = e.getMT();
			thread->wakeUp();
		}		

		bool resolved() const {
			return exception != nil || result != 0;
		}
	};

	//shortcut - if resolved, return value now
	if (getState() != IPromiseControl::stateNotResolved) {
		if (future->value != nil) return future->value;
		if (future->exception != nil) future->exception->throwAgain(THISLOCATION);
	}
	//shortcut - if not resolved and timeout is expired
	else if (tm.expired())
		//throw exception
		throw TimeoutException(THISLOCATION);

	//perform full waiting - create observer
	Ntf ntf(getCurThreadSleepingObj());
	//register observer
	future->registerObserver(&ntf);
	//wait until observer gets value or timeout
	while (!ntf.resolved() && !threadSleep(tm)) {}
	//remove observer
	if (!future->unregisterObserver(&ntf)) {
		//we were unable to unregister observer
		//because observer is currently running
		//just wait for finishing - without timeout - we cannot remove it!
		while (!ntf.resolved()) threadSleep(nil);
	}
	if (ntf.exception) ntf.exception->throwAgain();
	if (!ntf.result) throw TimeoutException(THISLOCATION);
	return *ntf.result;	
	
}

template<typename T>
const T & Future<T>::wait( ) const {
	return wait(Timeout());
}

template<typename T>
template<typename Fn>
Future<T> Future<T>::then( Fn fn) {
	if (future == nil) init();
	class X:public IObserver, public DynObject {
	public:
		X(Fn fn, const Promise<T> &rptr):fn(fn),rptr(rptr) {}
		virtual void resolve(const T &result) throw() {
			rptr.callAndResolve(fn, result);
			delete this;
		}
		virtual void resolve (const PException &e) throw() {
			rptr.reject(e);
			delete this;
		}

	protected:
		Fn fn; 
		Promise<T> rptr;
	};
	Future<T> p(future->alloc);
	addObserver(new(future->alloc) X(fn,p.getPromise()));
	return p;
}

template<typename T>
template<typename Fn>
Future<T> Future<T>::thenCall( Fn fn)
{
	if (future == nil) init();

	class X:public IObserver, public DynObject {
	public:
		X(Fn fn):fn(fn) {}
		virtual void resolve(const T &result) throw() {
			fn(result);
			delete this;
		}
		virtual void resolve(const PException &) throw() {
			delete this;
		}

	protected:
		Fn fn;
	};
	addObserver(new(future->alloc) X(fn));
	return *this;
}

template<typename T>
template<typename Fn>
Future<T> Future<T>::onException( Fn fn)
{
	if (future == nil) init();

	class X:public IObserver, public DynObject {
	public:
		X(Fn fn, const Promise<T> &rptr):fn(fn),rptr(rptr) {}
		virtual void resolve(const T &x) throw() {
			rptr.resolve(x);
			delete this;
		}
		virtual void resolve(const PException &oe) throw() {
			try {
				rptr.resolve(fn(oe));
			} catch (Exception &e) {
				rptr.reject(e << oe);
			} catch (std::exception &e) {
				rptr.reject(StdException(THISLOCATION,e) << oe);
			} catch (...) {
				rptr.reject(UnknownException(THISLOCATION) << oe);
			}
			delete this;
		}

	protected:
		Fn fn; 
		Promise<T> rptr;
	};
	Future<T> p(future->alloc);
	addObserver(new(future->alloc) X(fn,p.getPromise()));
	return p;
}

template<typename T>
template<typename Fn>
Future<T> Future<T>::onExceptionCall( Fn fn)
{
	if (future == nil) init();

	class X:public IObserver, public DynObject {
	public:
		X(Fn fn):fn(fn) {}
		virtual void resolve(const T &) throw() {
			delete this;
		}
		virtual void resolve(const PException &e) throw() {
			fn(e);
			delete this;
		}

	protected:
		Fn fn;
	};
	addObserver(new(future->alloc) X(fn));
	return *this;
}

template<typename T>
template<typename Fn, typename RFn>
Future<T> Future<T>::then(Fn resolveFn, RFn rejectFn) {
	if (future == nil) init();

	class X :public IObserver, public DynObject {
	public:
		X(Fn fn,RFn rfn,const Promise<T> &rptr):fn(fn),rfn(rfn),rptr(rptr) {}
		virtual void resolve(const T &result) throw() {
			try {
				rptr.resolve(fn(result));
			} catch (Exception &e) {
				rptr.reject(e);
			} catch (std::exception &e) {
				rptr.reject(StdException(THISLOCATION,e));
			} catch (...) {
				rptr.reject(UnknownException(THISLOCATION));
			}
			delete this;
		}
		virtual void resolve(const PException &oe) throw() {
			//deleted exception handler - error in reject is probihited
			rptr.resolve(rfn(oe));
			delete this;
		}

	protected:
		Fn fn; RFn rfn;Promise<T> rptr;
	};
	Future<T> p(future->alloc);
	addObserver(new(future->alloc) X(resolveFn,rejectFn,p.getPromise()));
	return p;

}

template<typename T>
void Future<T>::Resolution::resolve(Future<T> result) {
	class A : public IObserver, public DynObject {
	public:
		A(Promise<T> x) :x(x) {}

		virtual void resolve(const T &result) throw()
		{
			x.resolve(result);
			delete this;
		}

		virtual void resolve(const PException &e) throw()
		{
			x.resolve(e); delete this;

		}
		Promise<T> x;
	};
	Value *x = getValue();
	result.addObserver(new(x->alloc) A(Promise<T>(x)));
}

template<typename T>
template<typename Impl>
void Future<T>::Resolution::resolve( const Constructor<T, Impl> &result ) throw()
{
	resolve(VtConstructor<Constructor<T, Impl> >(result));
}


template<typename T>
void Future<T>::Resolution::resolve( const IConstructor<T> &result ) throw()
{
	Optional<T> temp;
	temp = result;
	resolve(temp);
}


template<typename T>
IPromiseControl::State Future<T>::Value::cancel( const PException &e ) throw() {	


	AutoArray<IObserver *, SmallAlloc<16> > cpy;
	{
		Synchronized<FastLock> _(lock);
		cpy.reserve(observers.length());
		while (!observers.empty()) {
			cpy.add(observers.getFront());
			observers.popFront();
		}
	}

	for (natural i = 0; i < cpy.length(); i++)
		cpy[i]->resolve(e);
		
	return getState();
	
}

template<typename T>
IPromiseControl::State Future<T>::Value::cancel() throw() {
	return cancel(new CanceledException(THISLOCATION));
}

template<typename T>
void Future<T>::clear() {
	future = nil;
}

template<typename T>
Promise<T> Future<T>::getPromise() {
	if (future == nil) clear(*getPromiseAlocator());
	return Promise<T>(future);
}

template<typename T>
void Future<T>::clear(IRuntimeAlloc &alloc) {
	future = new(alloc) Value(alloc);
	future = future.getMT();
}


template<typename T>
IPromiseControl::State Future<T>::cancel(const Exception &exception) throw() {
	if (future) return future->cancel(exception.clone()); else return IPromiseControl::stateResolved;
}

template<typename T>
IPromiseControl::State Future<T>::cancel(PException exception) throw()  {
	if (future) return future->cancel(exception); else return IPromiseControl::stateResolved;

}

template<typename T>
IPromiseControl::State Future<T>::cancel() throw()  {
	if (future) return future->cancel(); else return IPromiseControl::stateResolved;
}

template<typename T>
Future<T> Future<T>::isolate() {
	Future<T> newPromise(future->alloc);
	then(newPromise.getPromise());
	return newPromise;
}

template<typename T>
template<typename X>
X Future<T>::transformHelper(const T &value) {
	return X(value);
}


template<typename T>
template<typename X>
Future<T> Future<T>::transform(Future<X> original) {
	return transform(original,&transformHelper<X>);
}

template<typename T>
template<typename X, typename Fn>
Future<T> Future<T>::transform(Future<X> original, Fn fn) {

	class A:public Future<X>::Resolution, public DynObject {
	public:
		A(Fn fn, const Promise<T> &rptr):fn(fn),rptr(rptr) {}
		virtual void resolve(const X &result) throw() {
			try {
				rptr.resolve(fn(result));
			} catch (Exception &e) {
				rptr.reject(e);
			} catch (std::exception &e) {
				rptr.reject(StdException(THISLOCATION,e));
			} catch (...) {
				rptr.reject(UnknownException(THISLOCATION));
			}
			delete this;
		}
		virtual void resolve(const PException &oe) throw() {
			//deleted exception handler - error in reject is probihited
			rptr.reject(oe);
			delete this;
		}

	protected:
		Fn fn; Promise<T> rptr;
	};
	Future<T> p;
	original.registerCb(new(original.future->alloc) A(fn,p.createResult(original.future->alloc)));
	return p;
}

template<typename T>
void Future<T>::Value::addResultRef()
{
	lockInc(resultRefCnt);
}

template<typename T>
void Future<T>::Value::releaseResultRef()
{
	if (lockDec(resultRefCnt) == 0) {
		cancel();
	}
}


template<typename T>
Promise<T>::Promise(typename Future<T>::Value *r)
	:ptr(r)
{
	ptr->addResultRef();
}

template<typename T>
Promise<T>::~Promise()
{
	ptr->releaseResultRef();
}

template<typename T>
Promise<T>::Promise(const Promise &other)
:ptr(other.ptr)
{
	ptr->addResultRef();
}



template<typename T>
inline Future<T>::Value::~Value() {
	lock.lock();
	if (!observers.empty()) {
		CanceledException e(THISLOCATION);
		PException ce = e.clone();
		for (natural i = 0; i < observers.length();i++)
			observers[i]->resolve(ce);
	}
}


template<typename T>
Future<T> Future<T>::thenWake(ISleepingObject &sleep, natural resolveReason, natural rejectReason) {

	class A: public Resolution, public DynObject {
	public:
		A(ISleepingObject &sleepObj, natural resolved, natural rejected)
			:sleepObj(sleepObj),resolved(resolved),rejected(rejected) {}

		virtual void resolve(const T &) throw() {
			sleepObj.wakeUp(resolved);
			delete this;
		}
		virtual void resolve(const PException &) throw() {
			sleepObj.wakeUp(rejected);
			delete this;
		}

	protected:
		ISleepingObject &sleepObj;
		natural resolved;
		natural rejected;

	};
	addObserver(new(future->alloc) A(sleep,resolveReason,rejectReason));
	return *this;

}


template<typename Fn>
Future<void> Future<void>::then(Fn fn) {
	return Future<Empty>::then(EmptyCallVoid<Fn>(fn));
}

template<typename Fn>
Future<void> Future<void>::thenCall(Fn fn) {
	return Future<Empty>::thenCall(EmptyCallVoid<Fn>(fn));
}

template<typename Fn, typename RFn>
Future<void> Future<void>::then(Fn resolveFn, RFn rejectFn){
	return Future<Empty>::then(EmptyCallVoid<Fn>(resolveFn),rejectFn);
}

template<typename T>
inline Future<T> Future<T>::then(const Promise<T>& result) {
	class A: public IObserver, public DynObject {
	public:
		A(const Promise<T>& resolution)
			:resolution(resolution) {}

		virtual void resolve(const T &v) throw() {
			resolution.resolve(v);
			delete this;
		}
		virtual void resolve(const PException &e) throw() {
			resolution.resolve(e);
			delete this;
		}

	protected:
		Promise<T> resolution;

	};
	addObserver(new(future->alloc) A(result));
	return *this;

}

template<typename X>
Future<void> Future<void>::transform(Future<X> original)
{
	class A:public Future<X>::IObserver, public DynObject {
	public:
		A(Promise<void> &rptr):rptr(rptr) {}
		virtual void resolve(const X &) throw() {
			rptr.resolve();
			delete this;
		}
		virtual void resolve(const PException &oe) throw() {
			rptr.reject(oe);
			delete this;
		}

	protected:
		Promise<void> rptr;
	};
	Future<void> p(original.future->alloc);
	original.addObserver(new(original.future->alloc) A(p.getPromise()));
	return p;
}


template<typename T>
void Future<T>::Value::wait(const Timeout &tm) const
{
	Future<T> me(const_cast<Value *>(this));
	me.wait(tm);
}


template<typename T>
template<typename Fn>
void Promise<T>::callAndResolve(Fn fn) throw() {
	try {
		resolve(fn());
	} catch (const Exception &e) {
		reject(e);
	} catch (const std::exception &e) {
		reject(StdException(THISLOCATION,e));
	} catch (...) {
		reject(UnknownException(THISLOCATION));
	}
}

template<typename Fn>
void Promise<void>::callAndResolve(Fn fn) throw() {
	try {
		fn();
		resolve();
	} catch (const Exception &e) {
		reject(e);
	} catch (const std::exception &e) {
		reject(StdException(THISLOCATION,e));
	} catch (...) {
		reject(UnknownException(THISLOCATION));
	}

}

template<typename T>
template<typename Fn, typename Arg>
void Promise<T>::callAndResolve(Fn fn, Arg arg) throw() {
	try {
		resolve(fn(arg));
	} catch (const Exception &e) {
		reject(e);
	} catch (const std::exception &e) {
		reject(StdException(THISLOCATION,e));
	} catch (...) {
		reject(UnknownException(THISLOCATION));
	}
}

template<typename Fn, typename Arg>
void Promise<void>::callAndResolve(Fn fn, Arg arg) throw() {
	try {
		fn(arg);
		resolve();
	} catch (const Exception &e) {
		reject(e);
	} catch (const std::exception &e) {
		reject(StdException(THISLOCATION,e));
	} catch (...) {
		reject(UnknownException(THISLOCATION));
	}

}


template<typename T>
void LightSpeed::Future<T>::init()
{
	clear(*getPromiseAlocator());
}

template<typename T>
PPromiseControl Future<T>::getControlInterface()
{
	if (future == nil) init();
	return PPromiseControl(future);
}

template<typename T>
Future<T>::Future(IRuntimeAlloc &alloc) {
	clear(alloc);
}

template<typename T>
Future<T>::Future() {

}

template<typename T>
Promise<T>::Promise(const Future<T> &future) :ptr(future.getValuePtr())
{
	ptr->addResultRef();
}


template<typename T>
IPromiseControl::State Future<T>::getState() const throw () 
{
	if (future == nil) return IPromiseControl::stateNotResolved;
	return future->getState();
}

}

