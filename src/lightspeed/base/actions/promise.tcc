#pragma once

#include "promise.h"


#include "../containers/autoArray.tcc"
#include "../exceptions/canceledException.h"
#include "../exceptions/pointerException.h"

namespace LightSpeed {

template<typename T>
bool Promise<T>::Future::resolved() const throw()
{
	return (value != nil || exception != nil);
}

template<typename T>
void Promise<T>::Future::resolve( const T &result ) throw()
{
	resolveInternal<T>(result);
}

template<typename T>
void Promise<T>::Future::resolve( const IConstructor<T> &result ) throw ()
{
	resolveInternal<IConstructor<T> >(result);
}

template<typename T>
void Promise<T>::Future::reject(const PException &e ) throw ()
{
	{
		Synchronized<FastLock> _(lock);
		if (resolved()) return;
		exception = e.getMT();
	}
	for (natural i = 0; i < observers.length();i++)
		observers[i]->reject(exception);
	observers.clear();
	RefCntPtr<Future>(this).manualRelease();
}

template<typename T>
void Promise<T>::Future::registerObserver( IObserver *ifc )
{
	lock.lock();
	if (value != nil) {
		lock.unlock();
		ifc->resolve(value);
	}
	if (exception != nil) {
		lock.unlock();
		exception->throwAgain(THISLOCATION);
	}
	observers.add(ifc);
	lock.unlock();
}

template<typename T>
bool Promise<T>::Future::unregisterObserver( IObserver *ifc )
{
	Synchronized<FastLock> _(lock);
	for (natural i = observers.length(); i > 0 ;) {
		--i;
		if (observers[i] == ifc) {
			observers.erase(i); 
			return true;
		}
	}
	return false;
}

template<typename T>
template<typename X>
void Promise<T>::Future::resolveInternal( const X & result )
{
	{
		Synchronized<FastLock> _(lock);
		if (resolved()) return;
		value = result;
	}
	for (natural i = 0; i < observers.length();i++)
		observers[i]->resolve(value);

	observers.clear();
}


template<typename T>
void Promise<T>::addObserver( IObserver*ifc )
{
	if (future == nil) {
		throw NullPointerException(THISLOCATION);
	}
	future->registerObserver(ifc);
}

template<typename T>
bool Promise<T>::removeObserver( IObserver *ifc )
{
	if (future == nil) {
		throw NullPointerException(THISLOCATION);
	}
	return future->unregisterObserver(ifc);
}

template<typename T>
const T & Promise<T>::wait( const Timeout &tm /*= Timeout()*/ ) const
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

		virtual void reject(const PException &e ) throw()
		{
			this->exception = e.getMT();
			thread->wakeUp();
		}		

		bool resolved() const {
			return exception != nil || result != 0;
		}
	};

	Ntf ntf(getCurThreadSleepingObj());
	future->registerObserver(&ntf);
	while (!ntf.resolved() && !threadSleep(tm)) {}
	future->unregisterObserver(&ntf);
	if (ntf.exception) ntf.exception->throwAgain();
	if (!ntf.result) throw TimeoutException(THISLOCATION);
	return *ntf.result;	
	
}

template<typename T>
const T & Promise<T>::wait( ) const {
	return wait(Timeout());
}

template<typename T>
template<typename Fn>
Promise<T> Promise<T>::then( Fn fn) {
	class X:public IObserver, public DynObject {
	public:
		X(Fn fn, const Result &rptr):fn(fn),rptr(rptr) {}
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
		virtual void reject(const PException &e) throw() {
			rptr.reject(e);
			delete this;
		}

	protected:
		Fn fn; Result rptr;
	};
	Promise<T> p;
	addObserver(new(future->alloc) X(fn,p.createResult(future->alloc)));
	return p;
}

template<typename T>
template<typename Fn>
Promise<T> Promise<T>::thenCall( Fn fn)
{
	class X:public IObserver, public DynObject {
	public:
		X(Fn fn):fn(fn) {}
		virtual void resolve(const T &result) throw() {
			fn(result);
			delete this;
		}
		virtual void reject(const PException &) throw() {
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
Promise<T> Promise<T>::whenRejected( Fn fn)
{
	class X:public IObserver, public DynObject {
	public:
		X(Fn fn, const Result &rptr):fn(fn),rptr(rptr) {}
		virtual void resolve(const T &) throw() {
			delete this;
		}
		virtual void reject(const PException &oe) throw() {
			try {
				rptr.reject(fn(oe));
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
		Fn fn; Result rptr;
	};
	Promise<T> p;
	addObserver(new(future->alloc) X(fn,p.createResult(future->alloc)));
	return p;
}

template<typename T>
template<typename Fn>
Promise<T> Promise<T>::whenRejectedCall( Fn fn)
{
	class X:public Resolution, public DynObject {
	public:
		X(Fn fn):fn(fn) {}
		virtual void resolve(const T &) throw() {
			delete this;
		}
		virtual void reject(const PException &e) throw() {
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
Promise<T> Promise<T>::then(Fn resolveFn, RFn rejectFn) {
	class X:public Resolution, public DynObject {
	public:
		X(Fn fn,RFn rfn,Result rptr):fn(fn),rfn(rfn),rptr(rptr) {}
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
		virtual void reject(const PException &oe) throw() {
			//deleted exception handler - error in reject is probihited
			rptr.reject(rfn(oe));
			delete this;
		}

	protected:
		Fn fn; RFn rfn;Result rptr;
	};
	Promise<T> p;
	addObserver(new(future->alloc) X(resolveFn,rejectFn,p.createResult(future->alloc)));
	return p;

}

template<typename T>
void Promise<T>::Resolution::resolve(Promise<T> result) {
	result.addObserver(this);
}

template<typename T>
void Promise<T>::Resolution::reject(Promise<T> result) {
	result.addObserver(this);
}

template<typename T>
template<typename Impl>
void Promise<T>::Resolution::resolve( const Constructor<T, Impl> &result ) throw()
{
	resolve(VtConstructor<Constructor<T, Impl> >(result));
}


template<typename T>
void Promise<T>::Resolution::resolve( const IConstructor<T> &result ) throw()
{
	Optional<T> temp;
	temp = result;
	resolve(temp);
}


template<typename T>
void Promise<T>::Resolution::reject( const Exception &e ) throw() {
	reject(PException(e.clone()));

}

template<typename T>
void Promise<T>::Future::cancel( const PException &e ) throw() {	
		Observers cpy;
		{
			Synchronized<FastLock> _(lock);
			if (resolved()) return;
			cpy.swap(observers);
		}
		for (natural i = 0; i < cpy.length(); i++)
			cpy[i]->reject(e);
	
}

template<typename T>
void Promise<T>::Future::cancel( ) throw() {
	cancel(new CanceledException(THISLOCATION));
}

template<typename T>
void Promise<T>::init() {
	IRuntimeAlloc *alloc = getPromiseAlocator();
	future = new(*alloc) Future(*alloc);
	future = future.getMT();
}

template<typename T>
typename Promise<T>::Result Promise<T>::createResult() {
	if (future == nil || readAcquire(&future->resultRefCnt) != 0) {
		init();
	}
	return Result(future);
}

template<typename T>
void Promise<T>::init(IRuntimeAlloc &alloc) {
	future = new(alloc) Future(alloc);
	future = future.getMT();
}

template<typename T>
typename Promise<T>::Result Promise<T>::createResult(IRuntimeAlloc &alloc) {
	if (future == nil || readAcquire(&future->resultRefCnt) != 0) {
		init(alloc);
	}
	return Result(future);
}

template<typename T>
void Promise<T>::cancel(const Exception &exception) throw() {
	future->cancel(exception.clone());
}

template<typename T>
void Promise<T>::cancel(PException exception) throw()  {
	future->cancel(exception);

}

template<typename T>
void Promise<T>::cancel() throw()  {
	future->cancel();
}

template<typename T>
Promise<T> Promise<T>::isolate() {
	Promise<T> newPromise;
	then(newPromise.createResult(future->alloc));
	return newPromise;
}

template<typename T>
template<typename X>
X Promise<T>::transformHelper(const T &value) {
	return X(value);
}


template<typename T>
template<typename X>
Promise<T> Promise<T>::transform(Promise<X> original) {
	return transform(original,&transformHelper<X>);
}

template<typename T>
template<typename X, typename Fn>
Promise<T> Promise<T>::transform(Promise<X> original, Fn fn) {

	class A:public Promise<X>::Resolution, public DynObject {
	public:
		A(Fn fn, const Result &rptr):fn(fn),rptr(rptr) {}
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
		virtual void reject(const PException &oe) throw() {
			//deleted exception handler - error in reject is probihited
			rptr.reject(oe);
			delete this;
		}

	protected:
		Fn fn; Result rptr;
	};
	Promise<T> p;
	original.registerCb(new(original.future->alloc) A(fn,p.createResult(original.future->alloc)));
	return p;
}

template<typename T>
void Promise<T>::Future::addResultRef()
{
	if (lockInc(resultRefCnt) == 1) {
		RefCntPtr<Future> x(this);
		x.manualAddRef();
	}
}

template<typename T>
void Promise<T>::Future::releaseResultRef()
{
	if (lockDec(resultRefCnt) == 0) {
		RefCntPtr<Future> x(this);
		x.manualRelease();
		cancel();
	}
}


template<typename T>
Promise<T>::Result::Result(Future *r)
	:ptr(r)
{
	ptr->addResultRef();
}

template<typename T>
Promise<T>::Result::~Result()
{
	ptr->releaseResultRef();
}

template<typename T>
Promise<T>::Result::Result(const Result &other)
:ptr(other.ptr)
{
	ptr->addResultRef();
}



template<typename T>
inline Promise<T>::Future::~Future() {
	lock.lock();
	if (!observers.empty()) {
		CanceledException e(THISLOCATION);
		for (natural i = 0; i < observers.length();i++)
			observers[i]->reject(exception);
	}
}


template<typename T>
Promise<T> Promise<T>::thenWake(ISleepingObject &sleep, natural resolveReason, natural rejectReason) {

	class A: public Resolution, public DynObject {
	public:
		A(ISleepingObject &sleepObj, natural resolved, natural rejected)
			:sleepObj(sleepObj),resolved(resolved),rejected(rejected) {}

		virtual void resolve(const T &) throw() {
			sleepObj.wakeUp(resolved);
			delete this;
		}
		virtual void reject(const PException &) throw() {
			sleepObj.wakeUp(rejected);
			delete this;
		}

	protected:
		ISleepingObject &sleepObj;
		natural resolved;
		natural rejected;

	};
	registerCb(new(future->alloc) A(sleep,resolveReason,rejectReason));
	return *this;

}


template<typename T>
template<typename X>
Promise<X> Promise<T>::operator && (const Promise<X> &other) {

	class B: public Resolution, public DynObject {
	public:
		B(const X &value, const Result &res)
			:value(value),res(res) {}

		virtual void resolve(const T &) throw() {
			//ignore value and resolve next promise by stored value
			res.resolve(value);
			//delete this callback
			delete this;
		}
		virtual void reject(const PException &e) throw() {
			//reject the promise using exception
			res.reject(e);
			//delete this callback (with the result)
			delete this;
		}
	protected:
		X value;
		Result res;
	};

	class A: public Promise<X>::Resolution, public DynObject {
	public:
		A(const Promise<T> &checkPromise, const typename Promise<X>::Result &res)
			:checkPromise(checkPromise),res(res) {}

		virtual void resolve(const X &v) throw() {
			//create new callback which resolves once the other promise is resolved
			//register it
			checkPromise.registerCb(new(checkPromise.future->alloc) B(v,res));
			//delete this callback
			delete this;
		}
		virtual void reject(const PException &e) throw() {
			//forward rejection
			res.reject(e);
			delete this;
		}


	protected:
		Promise<T> checkPromise;
		typename Promise<X>::Result res;
	};


	Promise<X> p;
	other.registerCb(new(other.future->alloc) A(*this,p.createResult(other.future->alloc)));
	return p;
}

/*template<typename T>
Promise<T> Promise<T>:: operator || (const Promise<T> &other)  {

}*/


template<typename Fn>
Promise<void> Promise<void>::then(Fn fn) {
	return Promise<Empty>::then(EmptyCallVoid<Fn>(fn));
}

template<typename Fn>
Promise<void> Promise<void>::thenCall(Fn fn) {
	return Promise<Empty>::thenCall(EmptyCallVoid<Fn>(fn));
}

template<typename Fn, typename RFn>
Promise<void> Promise<void>::then(Fn resolveFn, RFn rejectFn){
	return Promise<Empty>::then(EmptyCallVoid<Fn>(resolveFn),rejectFn);
}

template<typename T>
inline Promise<T> Promise<T>::then(const Result& result) {
	class A: public IObserver, public DynObject {
	public:
		A(const Result& resolution)
			:resolution(resolution) {}

		virtual void resolve(const T &v) throw() {
			resolution.resolve(v);
			delete this;
		}
		virtual void reject(const PException &e) throw() {
			resolution.reject(e);
			delete this;
		}

	protected:
		Result resolution;

	};
	addObserver(new(future->alloc) A(result));
	return *this;

}



template<typename X>
Promise<void> Promise<void>::transform(Promise<X> original)
{
	class A:public Promise<X>::IObserver, public DynObject {
	public:
		A(Result &rptr):rptr(rptr) {}
		virtual void resolve(const X &) throw() {
			rptr.resolve();
			delete this;
		}
		virtual void reject(const PException &oe) throw() {
			rptr.reject(oe);
			delete this;
		}

	protected:
		Result rptr;
	};
	Promise<void> p;
	original.addObserver(new(original.future->alloc) A(p.createResult(original.future->alloc)));
	return p;
}


}

