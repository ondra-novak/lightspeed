/*
 * dispatcher.tcc
 *
 *  Created on: 29. 7. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_
#define LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_

#include "dispatcher.h"
#include "../../base/memory/cloneable.h"
#include "../exceptions/stdexception.h"
namespace LightSpeed {


template<typename T, typename Fn>
Promise<T> AbstractDispatcher::dispatch(const Promise<T> &, Fn fn) {

	class Action: public IDispatchAction {
	public:

		LIGHTSPEED_CLONEABLECLASS;
		Action(const Fn fn, const PromiseResolution<T> &res):fn(fn),res(res) {}
		void run() throw() {
			try {
				res.resolve((T)fn());
			} catch (const Exception &e) {
				res.reject(e.clone());
			} catch (const std::exception &e) {
				res.reject(new StdException(THISLOCATION,e));
			} catch (...) {
				res.reject(new UnknownException(THISLOCATION));
			}
		}
		void reject(const Exception &e) throw() {
			res.reject(e.clone());
		}

	protected:
		Fn fn;
		PromiseResolution<T> res;
	};

	PromiseResolution<T> pres;
	Promise<T> promise;
	Action a(fn,pres);
	const IDispatchAction &da = a;
	dispatch(da);
	return promise;
}


template<typename T> class AbstractDispatcher::PromiseDispatch: public Promise<T>::Resolution, public DynObject {
public:

	class DispatchResult: public IDispatchAction {
	public:
		LIGHTSPEED_CLONEABLECLASS;
		PromiseResolution<T> pr;
		T result;
		DispatchResult(PromiseResolution<T> pr,	T result):pr(pr),result(result) {}

		virtual void run() throw() {
			pr.resolve(result);
		}
		virtual void reject(const Exception &e) throw() {
			pr.reject(e.clone());
		}
	};

	class DispatchReject: public IDispatchAction {
	public:
		LIGHTSPEED_CLONEABLECLASS;
		PromiseResolution<T> pr;
		PException result;
		DispatchReject(PromiseResolution<T> pr,	PException result):pr(pr),result(result) {}

		virtual void run() throw() {
			pr.reject(result);
		}
		virtual void reject(const Exception &e) throw() {
			pr.reject(e.clone());
		}
	};

	PromiseDispatch(AbstractDispatcher &dispatcher, PromiseResolution<T> pr):dispatcher(dispatcher),pr(pr) {
	}

	virtual void resolve(const T &result) throw() {
		dispatcher.dispatch(DispatchResult(pr,result));
		delete this;
	}
	virtual void reject(const PException &e) throw() {
		dispatcher.dispatch(DispatchReject(pr,e));
		delete this;
	}


protected:
	AbstractDispatcher &dispatcher;
	PromiseResolution<T> pr;

};


template<typename T>
Promise<T> AbstractDispatcher::dispatch(Promise<T> promise) {
	PromiseResolution<T> pr;
	Promise<T> ret(pr);
	promise.registerCb(new(*getPromiseAlocator()) PromiseDispatch<T>(*this,pr));
	return ret;


}


}



#endif /* LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_ */
