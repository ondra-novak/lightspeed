/*
 * dispatcher.h
 *
 *  Created on: 28. 7. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ACTIONS_DISPATCHER_H_
#define LIGHTSPEED_BASE_ACTIONS_DISPATCHER_H_
#include "../exceptions/exception.h"
#include "promise.h"


namespace LightSpeed {

///IDispatcher - dispatches function call or promise to the executor - thread, thread pool or message queue
/** This is just interface and set of helper functions to convert templates to allow requeste be able passable through
 * the  virtual functions.
 *
 * To implement this interface, one should extend AbstractDispatcher, which contains some useful
 * common code.
 */
class IDispatcher {

	enum Tag_Function {tag_function};
	enum Tag_Promise {tag_promise};


	//if you receive error about this class, feature you requested is not available in C++03
	template <typename Fn>
	class DispatchHelper;

#if __cplusplus >= 201103L
	template <typename Fn>
	class DispatchHelper
	{
	public:
		typedef decltype(Fn::operator()) RetV;
		static const Tag_Function tag = tag_function;
	};
#endif

	template<typename T>
	class DispatchHelper<T (*)()> {
	public:
		typedef T RetV;
		static const Tag_Function tag = tag_function;
	};

	template<typename T>
	class DispatchHelper<Promise<T> > {
	public:
		typedef T RetV;
		static const Tag_Promise tag = tag_promise;
	};


public:

	///Generic action
	class IDispatchAction: public ICloneable {
	public:
		LIGHTSPEED_CLONEABLEDECL(IDispatchAction);
		virtual void run() throw() = 0;
		virtual void reject(const Exception &e) throw() = 0;
	};


	///Dispatches action through the object
	/** Depend on type of the object, action can be dispatched by many ways. If
	 *  dispatcher is queue, action is enqueued and processed later. If dispatcher
	 *  is paralel executor, new thread is created.
	 *
	 *  Function can block, if the dispatcher is busy.
	 *
	 *
	 * @param action action to dispatch. Function can create copy of the action.
	 *
	 * @note function must be MT safe
	 *
	 */
	virtual void dispatchAction(const IDispatchAction &action) = 0;


	///Dispatches the object through the dispatcher
	/**
	 * @param arg object to dispatch. The object can be either a function without arguments,
	 *     or it can be also function object, but this will work in C++11 and highter. It
	 *     can be also lambda function from C++11. Finally, it can be also Promise object which
	 *     will be dispatched later once promise is resolved.
	 *
	 *
	 * @return Returns promise which is resolved, once the dispatcher processes the object. Promise
	 * is resolved by return value of the function called on the object.
	 *
	 * @note if you need to dispatch function object in C++03, call dispatch() version with
	 *  two arguments
	 */
	template<typename Arg>
	Promise<typename DispatchHelper<Arg>::RetV> dispatch(const Arg &arg);


	///Dispatches the member function call
	/**
	 * @param obj object which member function should be called. Note that object is COPIED
	 *  to the dispatcher. If you want to avoid copying, construct proxy object that will
	 *  carry the pointer.
	 *
	 * @param memberfn Pointer to member function in format &Object::memberFn that will
	 * be dispatched. If you want to dispatch function object, use &Object::operator().
	 *
	 * @return Returns Promise which is resolved, once the dispatcher returns from the function. Promise
	 * is resolved by return value of the function called on the object.
	 */
	template<typename Object, typename RetVal>
	Promise<RetVal> dispatch(const Object &obj, RetVal (Object::*memberfn)());


	///Dispatches the member function call with and argument
	/**
	 * @param obj object which member function should be called. Note that object is COPIED
	 *  to the dispatcher. If you want to avoid copying, construct proxy object that will
	 *  carry the pointer.
	 *
	 * @param memberfn Pointer to member function in format &Object::memberFn that will
	 * be dispatched. If you want to dispatch function object, use &Object::operator().
	 *
	 * @param  arg user defined argument
	 *
	 * @return Returns Promise which is resolved, once the dispatcher returns from the function. Promise
	 * is resolved by return value of the function called on the object.
	 */
	template<typename Object, typename RetVal, typename Arg>
	Promise<RetVal> dispatch(const Object &obj, RetVal (Object::*memberfn)(Arg arg), const typename FastParam<Arg>::T arg);


protected:

	virtual void promiseRegistered(PPromiseControl ppromise) = 0;


	template<typename T> class PromiseDispatch;

	template<typename Arg>
	Promise<typename DispatchHelper<Arg>::RetV> dispatch2(const Arg &arg, Tag_Function);
	template<typename Arg>
	Promise<typename DispatchHelper<Arg>::RetV> dispatch2(const Arg &arg, Tag_Promise);


};

///Abstract dispatcher implements some common but very low level features
/** It for example handles Promise dispatching and registration.
 *
 * Most of dispatch requests are performed immediatelly in time of execution, so it
 * expects, that program will not try to access dispatcher when it is being destroyed
 * or has been destroyed.
 *
 * But if you dispatch a Promise, program cannot rely on that promise will be resolved
 * before the live of the dispatcher reaches its final destination.
 *
 * So if you dispatch Promise, it is registered on the dispatcher and once dispatcher
 * is being destroyed, all registered promises are canceled and detached, so future
 * resolution cannot access to already destroyed dispatcher.
 */
class AbstractDispatcher: public IDispatcher {
public:
	AbstractDispatcher();

	virtual ~AbstractDispatcher();

protected:
	typedef RefCntPtr<IPromiseControl> PPromiseControl;
	typedef AutoArray<PPromiseControl> DispatchedPromises;


	///called when promise is registered on the dispatcher
	/**
	 * @param ppromise pointer to interface that control the promise
	 *
	 * There is no function handling removing registration. Dispatcher
	 * can remove registration after promise is resolved, but not necesery immediatelly.
	 * Dispatcher time to time checks all registered promisses and removes resolved ones.
	 */
	void promiseRegistered(PPromiseControl ppromise);
	///Check and removes resolved promises when it is necesery
	void pruneRegPromises();
	///Cancels all unresolved promises
	/** Function is called in the destructor, but you should call it sooner to prevent
	 * posting messages while message queue is being destroyed. Once all promises are canceled,
	 * none of them are dispatched.
	 */
	void cancelAllPromises();

	DispatchedPromises dprom;
	FastLock lock;
	natural nextCheck;

};



}


#endif /* LIGHTSPEED_BASE_ACTIONS_DISPATCHER_H_ */
