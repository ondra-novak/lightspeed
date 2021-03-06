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
/** This is just interface and set of helper functions to convert templates to allow request be able passed through
 * the  virtual functions.
 *
 * To implement this interface, one should extend AbstractDispatcher, which contains some useful
 * common code.
 *
 *
 */

class IDispatcher {

public:

	///Generic action
	class AbstractAction: public DynObject {
	public:
		virtual void run() throw() = 0;
		virtual void reject() throw() = 0;

		virtual ~AbstractAction() throw() {}

	};

	///dispatch handcrafted action
	/**
	 @param action action to dispatch. Action should be allocated through dispatcher's allocator (getActionAllocator()).
	 Function takes ownership of the action and it is responsible to release object after action is carried out 

	 @note function can throw an exception. However action must be destroyed before exception is thrown out
	*/
	   
	virtual void dispatch(AbstractAction *action) = 0;

	///Dispatches function call
	/**
	* @param fn function to execute in dispatcher
	* @param returnValue variable that receives return value. If you want to ensure, that observers will
	* be called in the dispatcher's thread, you have to attach them before dispatch()
	*
	*/
	template<typename Fn, typename T >
	void dispatch(const Fn &fn, const Promise<T> &returnValue);


	///dispatch promise
	/**
	 * Causes that observers are notified in dispatcher's thread instead of main thread. It
	 * works also if the promise is already resolved
	 *
	 * @param source source promise - can be already resolved
	 * @param returnValue Promise of another Promise. It should have observers attached already.
	 */
	template<typename T>
	void dispatchFuture(const Future<T> &source, const Promise<T> &returnValue);


	///Dispatch resolution of the promise through the dispatcher
	/**
	 * dispatcher just resolves promise by supplied value. Main benefit of this is
	 * that observers are called by the dispatcher and possible in its thread. So
	 * caller is not blocked and can continue.
	 *
	 * The Promise can be also canceled without support from the dispatcher. Canceled 
	 * Promise is still resolved, but without notifying the observers.
	 *
	 * @param promise promise to resolve by the dispatcher
	 * @param value of the promise
	 */

	template<typename T>
	void dispatch(const Promise<T> &promise, const T &value);

	///Dispatch resolution of the promise through the dispatcher
	/**
	* dispatcher just resolves promise by supplied value. Main benefit of this is
	* that observers are called by the dispatcher and possible in its thread. So
	* caller is not blocked and can continue.
	*
	* The Promise can be also canceled without support from the dispatcher. Canceled
	* Promise is still resolved, but without notifying the observers.
	*
	* @param promise promise to resolve
	*/
	void dispatch(const Promise<void> &promise);



	///Dispatch function call without generation promise
	/**
	 * @param fn function to execute.
	 *
	 * @note return value is ignored. Function should not throw an exception.
	 */
	template<typename Fn>
	void dispatch(Fn fn);

	///Returns allocator for actions
	/** You should allocate actions using this allocator */
	virtual IRuntimeAlloc &getActionAllocator() = 0;

protected:

	template<typename T>
	static T throwFn();

	template<typename T, typename P>
	class PromiseSetResult;


	virtual void promiseRegistered(PPromiseControl ppromise) = 0;
	virtual void promiseResolved(PPromiseControl ppromise) = 0;





};

}


#endif /* LIGHTSPEED_BASE_ACTIONS_DISPATCHER_H_ */
