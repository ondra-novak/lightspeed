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

template<typename Arg>
Promise<typename AbstractDispatcher::DispatchHelper<Arg>::RetV> AbstractDispatcher::dispatch(
		const Arg& arg) {
	return dispatch2(arg,DispatchHelper<Arg>::tag);
}

template<typename Arg>
inline Promise<typename AbstractDispatcher::DispatchHelper<Arg>::RetV> AbstractDispatcher::dispatch2(
		const Arg& arg, Tag_Function) {

	typedef typename AbstractDispatcher::DispatchHelper<Arg>::RetV T;

	class Action: public IDispatchAction {
	public:

		LIGHTSPEED_CLONEABLECLASS;
		Action(const Arg fn, const PromiseResolution<T> &res):fn(fn),res(res) {}
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
		PromiseResolution<T> res;
	};

	PromiseResolution<T> pres;
	Promise<T> promise(pres);
	Action a(arg,pres);
	const IDispatchAction &da = a;
	dispatchAction(da);
	return promise;
}






template<typename Object, typename RetVal>
inline Promise<RetVal> LightSpeed::AbstractDispatcher::dispatch(
		const Object& obj, RetVal (Object::*memberfn)()) {

	typedef RetVal T;
	typedef RetVal (Object::*Fn)();

	class Action: public IDispatchAction {
	public:

		LIGHTSPEED_CLONEABLECLASS;
		Action(const Object &obj,  Fn fn, const PromiseResolution<T> &res):obj(obj),fn(fn),res(res) {}
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
		PromiseResolution<T> res;
	};

	PromiseResolution<T> pres;
	Promise<T> promise(pres);
	Action a(obj,memberfn,pres);
	const IDispatchAction &da = a;
	dispatchAction(da);
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
		dispatcher.dispatchAction(DispatchResult(pr,result));
		delete this;
	}
	virtual void reject(const PException &e) throw() {
		dispatcher.dispatchAction(DispatchReject(pr,e));
		delete this;
	}


protected:
	AbstractDispatcher &dispatcher;
	PromiseResolution<T> pr;

};



template<typename Arg>
inline Promise<typename AbstractDispatcher::DispatchHelper<Arg>::RetV> AbstractDispatcher::dispatch2(
		const Arg& arg, Tag_Promise) {


	typedef typename AbstractDispatcher::DispatchHelper<Arg>::RetV T;

	Promise<T> src = arg;
	PromiseResolution<T> pr;
	Promise<T> ret(pr);
	src.registerCb(new(*getPromiseAlocator()) PromiseDispatch<T>(*this,pr));
	return ret;


}

template<typename Object, typename RetVal, typename Arg>
inline Promise<RetVal> LightSpeed::AbstractDispatcher::dispatch(
		const Object& obj, RetVal (Object::*memberfn)(Arg arg),
		const typename FastParam<Arg>::T arg) {


	typedef RetVal T;
	typedef RetVal (Object::*Fn)();

	class Action: public IDispatchAction {
	public:

		LIGHTSPEED_CLONEABLECLASS;
		Action(const Object &obj,  Fn fn, Arg arg, const PromiseResolution<T> &res)
				:obj(obj),fn(fn),arg(arg),res(res) {}
		void run() throw() {
			try {
				res.resolve((T)(obj.*fn)(arg));
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
		Arg arg;
		PromiseResolution<T> res;
	};

	PromiseResolution<T> pres;
	Promise<T> promise(pres);
	Action a(obj,memberfn,arg,pres);
	const IDispatchAction &da = a;
	dispatchAction(da);
	return promise;
}


}



#endif /* LIGHTSPEED_BASE_ACTIONS_DISPATCHER_TCC_ */
