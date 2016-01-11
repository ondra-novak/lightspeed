/*
 * dispatcher.tcc
 *
 *  Created on: 29. 7. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_
#define LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_

#include "idispatcher.h"
#include "../../base/memory/cloneable.h"
#include "../exceptions/stdexception.h"
#include "../exceptions/canceledException.h"
#include "promise.tcc"
namespace LightSpeed {


	template<typename Fn>
	void IDispatcher::dispatch(Fn fn)
	{
		class A : public AbstractAction {
		public:
			A(const Fn &fn) :fn(fn) {}

			virtual void run() throw() {
				fn();
			}
			virtual void reject() throw() {
				//do nothing
			}
			virtual ~A() throw () {}
		protected:
			Fn fn;
		};
		
		AbstractAction *aa = new(getActionAllocator()) A(fn);		
		dispatch(aa);
	}


	template<typename Fn, typename T>
	void IDispatcher::dispatch(const Fn &fn, const Promise<T> &returnValue)
	{
		class A : public AbstractAction {
		public:
			A(const Fn &fn, const Promise<T> &res) :fn(fn), res(res) {}
			virtual void run() throw() {
				res.callAndResolve(fn);
			}
			virtual void reject() throw() {
				res.reject(CanceledException(THISLOCATION));
			}
			~A() throw() {}
		protected:
			Fn fn;
			Promise<T> res;
		};

		AbstractAction *aa = new(getActionAllocator()) A(fn, returnValue);
		dispatch(aa);
	}

	template<typename T, typename P>
	class IDispatcher::PromiseSetResult : public IDispatcher::AbstractAction{
	public:
		PromiseSetResult(const T &value, const Promise<P> &res) :value(value), res(res) {}

		virtual void run() throw()
		{
			res.resolve(value);
		}

		virtual void reject() throw()
		{
			res.reject(CanceledException(THISLOCATION));
		}
		~PromiseSetResult() throw() {}
	protected:
		T value;
		Promise<P> res;
	};



	template<typename T>
	void IDispatcher::dispatchFuture(const Future<T> &source, const  Promise<T> &returnValue)
	{
		class Observer : public Future<T>::IObserver, public DynObject {
		public:
			Observer(IDispatcher &owner, const  Promise<T> &res)
				:owner(owner), res(res) {}
			virtual void resolve(const T &result) throw()
			{
				AbstractAction *aa = new PromiseSetResult<T, T>(result, res);				
				try {
					owner.dispatch(aa);
					delete this;
				}
				catch (...) {
					res.callAndResolve(&IDispatcher::throwFn<T>);
					delete this;
				}
			}
			virtual void resolve(const PException &e) throw()
			{
				AbstractAction *aa = new PromiseSetResult<PException, T>(e, res);
				try {
					owner.dispatch(aa);
					delete this;
				}
				catch (...){
					res.callAndResolve(&IDispatcher::throwFn<T>);
					delete this;
				}
			}
		protected:
			IDispatcher &owner;
			Promise<T> res;

		};

		Future<T> s = source;
		Future<T> newpromise;
		newpromise.addObserver(new(*getPromiseAlocator()) Observer(*this, returnValue));
		s.then(newpromise.getPromise());		
	}


	template<typename T>
	void IDispatcher::dispatch(const  Promise<T> &promise, const T &value)
	{
		class A : public AbstractAction {
		public:

			A(const Promise<void> &promise, const T &value) :promise(promise),value(value) {}
			virtual void run() throw()
			{
				promise.resolve();
			}

			virtual void reject() throw()
			{
				promise.reject(CanceledException(THISLOCATION));
			}
		protected:
			typename Future<T>::Promise promise;
			T value;
		};
		AbstractAction *aa = new(getActionAllocator()) A(promise,value);
		dispatch(aa);
	}


	template<typename T>
	T IDispatcher::throwFn() {
		throw;
	}
}



#endif /* LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_ */
