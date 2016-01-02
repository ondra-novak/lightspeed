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
	void IDispatcher::dispatch(const Fn &fn, const typename Promise<T> &returnValue)
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

		virtual void run() throw() override
		{
			res.resolve(value);
		}

		virtual void reject() throw() override
		{
			res.reject(CanceledException(THISLOCATION));
		}
	protected:
		T value;
		Promise<P> res;
	};



	template<typename T>
	void IDispatcher::dispatchFuture(const Future<T> &source, const typename Promise<T> &returnValue)
	{
		class Observer : public Future<T>::IObserver, public DynObject {
		public:
			Observer(IDispatcher &owner, const typename Promise<T> &res)
				:owner(owner), res(res) {}
			virtual void resolve(const T &result) throw() override
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
			virtual void resolve(const PException &e) throw() override
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
			typename Promise<T> res;

		};

		Future<T> s = source;
		Future<T> newpromise;
		newpromise.addObserver(new(*getPromiseAlocator()) Observer(*this, returnValue));
		s.then(newpromise.getPromise());		
	}


	template<typename T>
	void IDispatcher::dispatch(const typename Promise<T> &promise, const T &value)
	{
		class A : public AbstractAction {
		public:

			A(const typename Future<void>::Promise &promise, const T &value) :promise(promise),value(value) {}
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


}



#endif /* LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_ */
