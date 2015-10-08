/*
 * dispatcher.tcc
 *
 *  Created on: 29. 7. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_
#define LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_

#include "abstractDispatcher.h"
#include "../../base/memory/cloneable.h"
#include "../exceptions/stdexception.h"
namespace LightSpeed {

template<typename Arg>
Promise<typename IDispatcher::DispatchHelper<Arg>::RetV> IDispatcher::dispatch(
		const Arg& arg) {
	return dispatch2(arg,DispatchHelper<Arg>::tag);
}

template<typename Arg>
inline Promise<typename IDispatcher::DispatchHelper<Arg>::RetV> IDispatcher::dispatch2(
		const Arg& arg, Tag_Function) {

	typedef typename IDispatcher::DispatchHelper<Arg>::RetV T;

	class Action: public AbstractAction {
	public:

		LIGHTSPEED_CLONEABLECLASS;
		Action(const Arg fn, const typename Promise<T>::Result &res):fn(fn),res(res) {}
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
		Arg fn;
		typename Promise<T>::Result res;
	};

	Promise<T> promise;
	dispatchAction(new(getActionAllocator()) Action(arg,promise.createResult()));
	return promise;
}






template<typename Object, typename RetVal>
inline Promise<RetVal> LightSpeed::IDispatcher::dispatch(
		const Object& obj, RetVal (Object::*memberfn)()) {

	typedef RetVal T;
	typedef RetVal (Object::*Fn)();

	class Action: public AbstractAction {
	public:

		LIGHTSPEED_CLONEABLECLASS;
		Action(const Object &obj,  Fn fn, const typename Promise<T>::Result &res):obj(obj),fn(fn),res(res) {}
		void run() throw() {
			try {
				res.resolve((T)(obj.*fn)());
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
		Object obj;
		Fn fn;
		typename Promise<T>::Result res;
	};

	Promise<T> promise;
	dispatchAction(new(getActionAllocator()) Action(obj,memberfn,promise.createResult()));
	return promise;
}



template<typename T> class IDispatcher::PromiseDispatch: public Promise<T>::IObserver, public DynObject {
public:

	class DispatchResult: public AbstractAction {
	public:
		LIGHTSPEED_CLONEABLECLASS;
		typename Promise<T>::Result pr;
		T result;
		DispatchResult(const typename Promise<T>::Result &pr,	T result):pr(pr),result(result) {}

		virtual void run() throw() {
			pr.resolve(result);
		}
		virtual void reject(const Exception &e) throw() {
			pr.reject(e.clone());
		}
	};

	class DispatchReject: public AbstractAction {
	public:
		LIGHTSPEED_CLONEABLECLASS;
		typename Promise<T>::Result  pr;
		PException result;
		DispatchReject(const typename Promise<T>::Result &pr,	PException result):pr(pr),result(result) {}

		virtual void run() throw() {
			pr.reject(result);
		}
		virtual void reject(const Exception &e) throw() {
			pr.reject(e.clone());
		}
	};

	PromiseDispatch(IDispatcher &dispatcher, const typename Promise<T>::Result &pr):dispatcher(dispatcher),pr(pr) {
	}
	virtual void resolve(const T &result) throw() {
		dispatcher.dispatchAction(new(dispatcher.getActionAllocator()) DispatchResult(pr,result));
		delete this;
	}
	virtual void reject(const PException &e) throw() {
		dispatcher.dispatchAction(new(dispatcher.getActionAllocator()) DispatchReject(pr,e));
		delete this;
	}


protected:
	IDispatcher &dispatcher;
	const typename Promise<T>::Result pr;

};



template<typename Arg>
inline Promise<typename IDispatcher::DispatchHelper<Arg>::RetV> IDispatcher::dispatch2(
		const Arg& arg, Tag_Promise) {

	//origin -> isolated -> |dispatcher| -> returned -> (user callbacks)
	//
	//when dispatcher exits, isolated is canceled
	//
	//origin -> isolated
	//
	//this prevents asking the dispatcher after it is destroyed


	typedef typename IDispatcher::DispatchHelper<Arg>::RetV T;

	Promise<T> src = arg;
	Promise<T> isolated = src.isolate();
	Promise<T> ret;
	isolated.addObserver(new(*getPromiseAlocator()) PromiseDispatch<T>(*this,ret.createResult()));

	promiseRegistered(isolated.getControlInterface());

	return isolated;


}

namespace {
	template<typename Object, typename RetVal, typename Arg>
	class AAction : public AbstractDispatcher::AbstractAction {
	public:

		LIGHTSPEED_CLONEABLECLASS;
		AAction(const Object &obj, RetVal(Object::*fn)(Arg arg), Arg arg, const typename  Promise<RetVal>::Result &res)
			:obj(obj), fn(fn), arg(arg), res(res) {}
		void run() throw() {
			try {
				res.resolve((RetVal)(obj.*fn)(arg));
			}
			catch (const Exception &e) {
				res.reject(e.clone());
			}
			catch (const std::exception &e) {
				res.reject(new StdException(THISLOCATION, e));
			}
			catch (...) {
				res.reject(new UnknownException(THISLOCATION));
			}
		}
		void reject(const Exception &e) throw() {
			res.reject(e.clone());
		}

	protected:
		Object obj;
		RetVal(Object::*fn)(Arg arg);
		Arg arg;
		typename Promise<RetVal>::Result res;
	};

	template<typename Object, typename Arg>
	class AAction<Object,void,Arg> : public AbstractDispatcher::AbstractAction {
	public:

		LIGHTSPEED_CLONEABLECLASS;
		AAction(Object &obj, void(Object::*fn)(Arg arg), Arg arg, const typename  Promise<void>::Result &res)
			:obj(obj), fn(fn), arg(arg), res(res) {}
		void run() throw() {
			try {
				(obj.*fn)(arg);
				res.resolve();
			}
			catch (const Exception &e) {
				res.reject(e.clone());
			}
			catch (const std::exception &e) {
				res.reject(new StdException(THISLOCATION, e));
			}
			catch (...) {
				res.reject(new UnknownException(THISLOCATION));
			}
		}
		void reject(const Exception &e) throw() {
			res.reject(e.clone());
		}

	protected:
		Object &obj;
		void(Object::*fn)(Arg arg);
		Arg arg;
		typename Promise<void>::Result res;
	};

}

template<typename Object, typename RetVal, typename Arg>
inline Promise<RetVal> IDispatcher::dispatch(
		Object& obj, RetVal (Object::*memberfn)(Arg arg),
		const typename FastParam<Arg>::T arg) {


	typedef RetVal T;


	Promise<T> promise;
	dispatchAction(new(getActionAllocator())  AAction<Object,RetVal,Arg>(obj,memberfn,arg,promise.createResult()));
	return promise;
}


}



#endif /* LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_ */
