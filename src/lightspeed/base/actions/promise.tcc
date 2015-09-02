#pragma once

#include "promise.h"


#include "../containers/autoArray.tcc"
#include "../exceptions/canceledException.h"

namespace LightSpeed {

template<typename T>
bool Promise<T>::Future::checkResolved()
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
		if (checkResolved()) return;
		exception = e.getMT();
	}
	for (natural i = 0; i < sleepers.length();i++) 
		sleepers[i]->reject(exception);
	sleepers.clear();
	RefCntPtr<Future>(this).manualRelease();
}

template<typename T>
void Promise<T>::Future::registerExpectant( Resolution *ifc )
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
	sleepers.add(ifc);
	lock.unlock();
}

template<typename T>
void Promise<T>::Future::unregisterExpectant( Resolution *ifc )
{
	Synchronized<FastLock> _(lock);
	for (natural i = sleepers.length(); i > 0 ;) {
		--i;
		if (sleepers[i] == ifc) {
			sleepers.erase(i); break;
		}

	}
}

template<typename T>
template<typename X>
void Promise<T>::Future::resolveInternal( const X & result )
{
	{
		Synchronized<FastLock> _(lock);
		if (checkResolved()) return;
		value = result;
	}
	for (natural i = 0; i < sleepers.length();i++) 
		sleepers[i]->resolve(value);

	sleepers.clear();
	RefCntPtr<Future>(this).manualRelease();
}


template<typename T>
Promise<T> Promise<T>::registerCb( Resolution *ifc )
{
	future->registerExpectant(ifc);
	return *this;
}

template<typename T>
Promise<T> Promise<T>::unregisterCb( Resolution *ifc )
{
	future->unregisterExpectant(ifc);
	return *this;
}

template<typename T>
const T & Promise<T>::wait( const Timeout &tm /*= Timeout()*/ ) const
{
	class Ntf: public Resolution {
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
	future->registerExpectant(&ntf);
	while (!ntf.resolved() && !threadSleep(tm)) {}
	future->unregisterExpectant(&ntf);
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
	class X:public Resolution, public DynObject {
	public:
		X(Fn fn, PromiseResolution<T> rptr):fn(fn),rptr(rptr) {}
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
		Fn fn; PromiseResolution<T> rptr;
	};
	PromiseResolution<T> rptr;
	Promise<T> p(rptr,future->alloc);
	registerCb(new(future->alloc) X(fn,rptr));
	return p;
}

template<typename T>
template<typename Fn>
Promise<T> Promise<T>::thenCall( Fn fn)
{
	class X:public Resolution, public DynObject {
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
	registerCb(new(future->alloc) X(fn));
	return *this;
}

template<typename T>
template<typename Fn>
Promise<T> Promise<T>::whenRejected( Fn fn)
{
	class X:public Resolution, public DynObject {
	public:
		X(Fn fn, PromiseResolution<T> rptr):fn(fn),rptr(rptr) {}
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
		Fn fn; PromiseResolution<T> rptr;
	};
	PromiseResolution<T> rptr;
	Promise<T> p(rptr,future->alloc);
	registerCb(new(future->alloc) X(fn,rptr));
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
	registerCb(new(future->alloc) X(fn));
	return *this;
}

template<typename T>
template<typename Fn, typename RFn>
Promise<T> Promise<T>::then(Fn resolveFn, RFn rejectFn) {
	class X:public Resolution, public DynObject {
	public:
		X(Fn fn,RFn rfn, PromiseResolution<T> rptr):fn(fn),rfn(rfn),rptr(rptr) {}
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
		Fn fn; RFn rfn;PromiseResolution<T> rptr;
	};
	PromiseResolution<T> rptr;
	Promise<T> p(rptr,future->alloc);
	registerCb(new(future->alloc) X(resolveFn,rejectFn,rptr));
	return p;

}

template<typename T>
void Promise<T>::Resolution::resolve(Promise<T> result) {
	result.registerCb(this);
}

template<typename T>
void Promise<T>::Resolution::reject(Promise<T> result) {
	result.registerCb(this);
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
	Sleepers cpy;
	{
		Synchronized<FastLock> _(lock);
		cpy.swap(sleepers);
	}
	for (natural i = 0; i < cpy.length();i++)
		cpy[i]->reject(e);
}

template<typename T>
void Promise<T>::Future::cancel( ) throw() {
	cancel(new CanceledException(THISLOCATION));
}


template<typename T>
Promise<T>::Promise(PromiseResolution<T> &resolution) {
	IRuntimeAlloc *alloc = getPromiseAlocator();
	future = new(*alloc) Future(*alloc);
	future = future.getMT();
	future.manualAddRef();
	resolution = PromiseResolution<T>(future);

}

template<typename T>
Promise<T>::Promise(PromiseResolution<T> &resolution, IRuntimeAlloc &alloc) {
	future = new(alloc) Future(alloc);
	future = future.getMT();
	future.manualAddRef();
	resolution = PromiseResolution<T>(future);

}

template<typename T>
inline FastLock* Promise<T>::Future::getLockPtr() {
	return &lock;
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
	PromiseResolution<T> res;
	Promise<T> newPromise(res);
	return then(res);
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
		A(Fn fn, PromiseResolution<T> rptr):fn(fn),rptr(rptr) {}
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
		Fn fn; PromiseResolution<T> rptr;
	};
	PromiseResolution<T> rptr;
	Promise<T> p(rptr,original.future->alloc);
	original.registerCb(new(original.future->alloc) A(fn,rptr));
	return p;
}

template<typename T>
PromiseResolution<T>::~PromiseResolution(){
	if (!SharedResource::isShared() && ptr) {
		reject(CanceledException(THISLOCATION));
	} else {
		unshare(ptr?ptr->getLockPtr():0);
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
		B(const X &value, const PromiseResolution<X> &res)
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
		PromiseResolution<X> res;
	};

	class A: public Promise<X>::Resolution, public DynObject {
	public:
		A(const Promise<T> &checkPromise, const PromiseResolution<X> &res)
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
		PromiseResolution<X> res;
	};


	PromiseResolution<X> rptr;
	Promise<X> p(rptr,other.future->alloc);
	other.registerCb(new(other.future->alloc) A(*this,rptr));
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
inline Promise<T> Promise<T>::then(const PromiseResolution<T>& resolution) {
	class A: public Resolution, public DynObject {
	public:
		A(const PromiseResolution<T>& resolution)
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
		PromiseResolution<T> resolution;

	};
	registerCb(new(future->alloc) A(resolution));
	return *this;

}

Promise<void> Promise<void>::then(const PromiseResolution<void> &resolution) {
	return Promise<Empty>::then(static_cast<const PromiseResolution<Empty> &>(resolution));
}


template<typename X>
Promise<void> Promise<void>::transform(Promise<X> original)
{
	class A:public Promise<X>::Resolution, public DynObject {
	public:
		A(PromiseResolution<void> rptr):rptr(rptr) {}
		virtual void resolve(const X &) throw() {
			rptr.resolve();
			delete this;
		}
		virtual void reject(const PException &oe) throw() {
			rptr.reject(oe);
			delete this;
		}

	protected:
		PromiseResolution<void> rptr;
	};
	PromiseResolution<void> rptr;
	Promise<void> p(rptr,original.future->alloc);
	original.registerCb(new(original.future->alloc) A(rptr));
	return p;
}

}

