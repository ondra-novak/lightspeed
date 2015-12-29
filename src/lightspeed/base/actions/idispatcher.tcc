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
	void IDispatcher::dispatch(const Fn &fn, const typename Promise<T>::Result &returnValue)
	{
		class A : public AbstractAction {
		public:
			A(const Fn &fn, const typename Promise<T>::Result &res) :fn(fn), res(res) {}
			virtual void run() throw() {
				res.callAndResolve(fn);
			}
			virtual void reject() throw() {
				res.reject(CanceledException(THISLOCATION));
			}
		protected:
			Fn fn;
			typename Promise<T>::Result res;
		};

		AbstractAction *aa = new(getActionAllocator()) A(fn, returnValue);
		dispatch(aa);
	}

	template<typename T, typename P>
	class IDispatcher::PromiseSetResult : public IDispatcher::AbstractAction{
	public:
		PromiseSetResult(const T &value, const typename Promise<P>::Result &res) :value(value), res(res) {}

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
		typename Promise<P>::Result res;
	};



	template<typename T>
	void IDispatcher::dispatchPromise(const Promise<T> &source, const typename Promise<T>::Result &returnValue)
	{
		class Observer : public Promise<T>::IObserver, public DynObject {
		public:
			Observer(IDispatcher &owner, const typename Promise<T>::Result &res)
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
			typename Promise<T>::Result res;

		};

		Promise<T> s = source;
		Promise<T> newpromise;		
		newpromise.addObserver(new(*getPromiseAlocator()) Observer(*this, returnValue));
		s.then(newpromise.createResult(*getPromiseAlocator()));		
	}



}



#endif /* LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_ */
