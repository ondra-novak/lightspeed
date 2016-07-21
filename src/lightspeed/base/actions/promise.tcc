#pragma once

#include "promise.h"

#include "../../mt/atomic.h"

#include "../containers/deque.tcc"
#include "../containers/variant.h"
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
			//deleted exception handler - error in reject is prohibited
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
template<typename Fn, typename RFn>
Future<T> Future<T>::thenCall(Fn resolveFn, RFn rejectFn) {
	if (future == nil) init();

	class X :public IObserver, public DynObject {
	public:
		X(Fn fn,RFn rfn):fn(fn),rfn(rfn) {}
		virtual void resolve(const T &result) throw() {
			fn(result);
			delete this;
		}
		virtual void resolve(const PException &oe) throw() {
			rfn(oe);
			delete this;
		}

	protected:
		Fn fn; RFn rfn;
	};
	addObserver(new(future->alloc) X(resolveFn,rejectFn));
	return *this;

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
	return Future<Void>::then(EmptyCallVoid<Fn>(fn));
}

template<typename Fn>
Future<void> Future<void>::thenCall(Fn fn) {
	return Future<Void>::thenCall(EmptyCallVoid<Fn>(fn));
}


template<typename Fn, typename RFn>
Future<void> Future<void>::then(Fn resolveFn, RFn rejectFn){
	return Future<Void>::then(EmptyCallVoid<Fn>(resolveFn),rejectFn);
}

template<typename Fn, typename RFn>
Future<void> Future<void>::thenCall(Fn resolveFn, RFn rejectFn){
	return Future<Void>::thenCall(EmptyCallVoid<Fn>(resolveFn),rejectFn);
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

template<typename T>
bool Future<T>::Value::isLastReference() const {
	return (readAcquire(&this->counter) == readAcquire(&this->resultRefCnt)+1);
}

template<typename T>
FutureAutoCancel<T>::FutureAutoCancel(const Future<T> &f):Future<T>(f) {
}

template<typename T>
FutureAutoCancel<T>::~FutureAutoCancel() {

	if (this->future != null) {
		if (this->future->isLastReference()) {
			if (IPromiseControl::stateResolving == this->cancel()) {
				wait();
			}
		}
	}
}

namespace _intr {

template<typename From, typename To, typename Fn>
class FnResolveObserver: public Future<From>::IObserver, public DynObject {
public:
	FnResolveObserver(const Fn &fn, const Promise<To> &p):fn(fn),p(p) {}
	virtual void resolve(const From &result) throw() {
		p.callAndResolve(fn, result);
		delete this;
	}
	virtual void resolve(const PException &e) throw() {
		p.reject(e);
		delete this;
	}
protected:
	Fn fn;
	Promise<To> p;
};

template<typename To, typename Fn>
class FnResolveObserver<void, To, Fn>: public Future<void>::IObserver, public DynObject {
public:
	FnResolveObserver(const Fn &fn, const Promise<To> &p):fn(fn),p(p) {}
	virtual void resolve() throw() {
		p.callAndResolve(fn);
		delete this;
	}
	virtual void resolve(const PException &e) throw() {
		p.reject(e);
		delete this;
	}
protected:
	Fn fn;
	Promise<To> p;
};

template<typename T, typename Fn>
class FnResolveObserver<T,T,FutureCatch<Fn> >: public Future<T>::IObserver, public DynObject {
public:
	FnResolveObserver(const FutureCatch<Fn> &fn, const Promise<T> &p):fn(fn),p(p) {}
	virtual void resolve(const T &result) throw() {
		p.resolve(result);
		delete this;
	}
	virtual void resolve(const PException &e) throw() {
		p.callAndResolve(fn.fn, e);
		delete this;
	}
protected:
	FutureCatch<Fn> fn;
	Promise<T> p;
};


template<typename Fn>
class FnResolveObserver<void,void,FutureCatch<Fn> >: public Future<void>::IObserver, public DynObject {
public:
	FnResolveObserver(const FutureCatch<Fn> &fn, const Promise<void> &p):fn(fn),p(p) {}
	virtual void resolve() throw() {
		p.resolve();
		delete this;
	}
	virtual void resolve(const PException &e) throw() {
		p.callAndResolve(fn.fn, e);
		delete this;
	}
protected:
	FutureCatch<Fn> fn;
	Promise<void> p;
};

}

#if __cplusplus >= 201103L


template<typename T, typename Fn>
auto operator >> (Future<T> f, const Fn &fn)
	-> typename _intr::DetermineFutureType<typename _intr::DetermineFutureHandlerRetVal<T,Fn>::type>::Type {
	IRuntimeAlloc &alloc = f.getAllocator();
	typedef typename _intr::DetermineFutureType<typename _intr::DetermineFutureHandlerRetVal<T,Fn>::type>::Type FutT;
	typedef typename FutT::Type ToType;
	typedef T FromType;
	FutT p(alloc);
	f.addObserver(new(alloc) _intr::FnResolveObserver<FromType,ToType, Fn>(fn,p.getPromise()));
	return p;
}

#endif


template<typename T>
template<typename Y>
Future<Variant> Future<T>::operator||(const Future<Y> &b) {

	class TtoVariant: public Future<T>::IObserver {
	public:
		TtoVariant(const Promise<Variant> &v):v(v) {}
		virtual void resolve(const T &val) throw() {
			v.resolve(Variant(val));
			delete this;
		}
		virtual void resolve(const PException &val) throw() {
			v.resolve(val);
			delete this;
		}
		Promise<Variant> v;
	};
	class YtoVariant: public Future<Y>::IObserver {
	public:
		YtoVariant(const Promise<Variant> &v):v(v) {}
		virtual void resolve(const Y &val) throw() {
			v.resolve(Variant(val));
			delete this;
		}
		virtual void resolve(const PException &val) throw() {
			v.resolve(val);
			delete this;
		}
		Promise<Variant> v;
	};

	//retrieve alocator
	IRuntimeAlloc &alloc = getAllocator();
	//create future with allocator
	Future<Variant> v(alloc);
	//retrieve promise
	Promise<Variant> p = v.getPromise();

	//share promise between converters - only first will resolve, other will be ignored
	this->addObserver(new(alloc) TtoVariant(p));
	b.addObserver(new(alloc) YtoVariant(p));
	//return result promise
	return v;
}

template<typename T>
Future<T> Future<T>::operator||(const Future &b) {

	//retrieve alocator
	IRuntimeAlloc &alloc = getAllocator();
	//create future with allocator
	Future<Variant> v(alloc);
	//retrieve promise
	Promise<Variant> p = v.getPromise();

	//resolve promise when both futures are resolve- only first will resolve, other will be ignored
	this->then(p);
	b.then(p);
	//return result promise

	return v;
}

namespace _intr {
//Activates resolution, when both futures are resolved
/* It also contains temporary stored results
 * and also contains object observing the promises
 *
 * Ref counter tracks of references, everytime promise is resolved, reference is decreased
 */
template<typename T, typename Y>
class FutureAllAktivator: public DynObject, public RefCntObj {
public:

	//result 1
	Optional<T> v1;
	//result 2
	Optional<Y> v2;
	//result promise
	Promise<std::pair<T,Y> > result;




	//called to check whether both are resolved
	void tryResolve() {

		if (v1 != null && v2 != null) {
			//resolve pair
			result.resolve(std::pair<T,Y>(v1,v2));
		}
	}

	void reject(const PException &e) {
		//reject whole promise
		result.reject(e);
	}

	typedef RefCntPtr<FutureAllAktivator> PAktivator;

	class ReceiverX: public Future<T>::IObserver {
	public:
		PAktivator owner;

		ReceiverX(const PAktivator &owner):owner(owner.getMT()) {}
		virtual void resolve(const T &v) throw() {
			//store result
			owner->v1 = v;
			// try to resolve
			owner->tryResolve();
			// set pointer to NULL (decrease counter)
			owner = null;
		}
		virtual void resolve(const PException &e) throw() {
			//reject
			owner->reject(e);
			// set pointer to NULL (decrease counter)
			owner = null;
		}
	};

	class ReceiverY: public Future<Y>::IObserver {
	public:
		PAktivator owner;

		ReceiverY(const PAktivator &owner):owner(owner.getMT()) {}
		virtual void resolve(const Y &v) throw() {
			//store result
			owner.v2 = v;
			// try to resolve
			owner.tryResolve();
			// set pointer to NULL (decrease counter)
			owner = null;
		}
		virtual void resolve(const PException &e) throw() {
			//reject
			owner.reject(e);
			// set pointer to NULL (decrease counter)
			owner = null;
		}
	};

	ReceiverX recvX;
	ReceiverY recvY;

	FutureAllAktivator(const Promise<std::pair<T,Y> > &result)
		:result(result),recvX(*this),recvY(*this) {}


};

}

template<typename T>
template<typename Y>
Future<std::pair<T,Y> > Future<T>::operator&& (const Future<Y> &b) {


	IRuntimeAlloc &alloc = getAllocator();
	Future<std::pair<T,Y> > res(alloc);
	Promise<std::pair<T,Y> > p = res.getPromise();
	_intr::FutureAllAktivator<T,Y> *akt = new(alloc) _intr::FutureAllAktivator<T,Y>(p);
	try {
		this->addObserver(&akt->recvX);
		b.addObserver(&akt->recvY);
	} catch (...) {
		try {
			this->removeObserver(&akt->recvX);
			b.removeObserver(&akt->recvY);
		} catch (...) {
			//there shouldn't be exception;...
			std::terminate();
		}
		delete akt;
	}

	return res;
}



}

