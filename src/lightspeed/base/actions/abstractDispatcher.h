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
 */
class IDispatcher {

public:

	///Generic action
	class AbstractAction: public DynObject {
	public:
		virtual void run() throw() = 0;
		virtual void reject(const Exception &e) throw() = 0;

		virtual ~AbstractAction() {}

	};

	///dispatch handcrafted action
	virtual void dispatch(AbstractAction *action) = 0;


	///dispatch promise
	/**
	 * Causes that observers are notified in dispatcher's thread instead of main thread. It
	 * works also if the promise is already resolved
	 *
	 * @param source source promise - can be already resolved
	 * @param returnValue Result of another Promise. It should have observers attached already.
	 */
	template<typename T>
	void dispatch(Promise<T> source, typename Promise<T>::Result returnValue);


	///Dispatches function call
	/**
	 * @param fn function to execute in dispatcher
	 * @param returnValue variable that receives return value. If you want to ensure, that observers will
	 * be called in the dispatcher's thread, you have to attach them before dispatch()
	 *
	 */
	template<typename T, typename Fn>
	void dispatch(Fn fn, typename Promise<T>::Result returnValue);


	///Dispatch function call without generation promise
	/**
	 * @param fn function to execute.
	 *
	 * @note return value is ignored. Function should not throw an exception.
	 */
	template<typename Fn>
	void dispatch(Fn fn);

	///Dispatch member function call
	/**
	 * @param objptr pointer or smart pointer to object
	 * @param fn pointer to member function
	 * @param returnValue variable that receives result
	 */
	template<typename T, typename ObjPtr, typename RetVal, typename Obj>
	void dispatch(const ObjPtr &objptr, RetVal (Obj::*fn)(), typename Promise<T>::Result returnValue);

	///Dispatch member function call
	/**
	 * @param objptr pointer or smart pointer to object
	 * @param fn pointer to member function
	 * @param arg argument passed to the member function
	 * @param returnValue variable that receives result
	 */
	template<typename T, typename ObjPtr, typename RetVal, typename Obj, typename Arg>
	void dispatch(const ObjPtr &objptr, RetVal (Obj::*fn)(Arg), typename FastParam<Arg>::T arg, typename Promise<T>::Result returnValue);

	///Dispatch member function call returning void
	/**
	 * @param objptr pointer or smart pointer to object
	 * @param fn pointer to member function
	 * @param returnValue void-promise which becomes resolved once function finishes
	 */
	template<typename ObjPtr, typename RetVal, typename Obj>
	void dispatch(const ObjPtr &objptr, RetVal (Obj::*fn)(), typename Promise<void>::Result returnValue);

	///Dispatch member function call returning void
	/**
	 * @param objptr pointer or smart pointer to object
	 * @param fn pointer to member function
	 * @param arg argument passed to the member function
	 * @param returnValue void-promise which becomes resolved once function finishes
	 */
	template<typename ObjPtr, typename RetVal, typename Obj, typename Arg>
	void dispatch(const ObjPtr &objptr, RetVal (Obj::*fn)(Arg), typename FastParam<Arg>::T arg, typename Promise<void>::Result returnValue);



	///Returns allocator for actions
	/** You should allocate actions using this allocator */
	virtual IRuntimeAlloc &getActionAllocator() = 0;

protected:


	virtual void promiseRegistered(PPromiseControl ppromise) = 0;
	virtual void promiseResolved(PPromiseControl ppromise) = 0;





};


}


#endif /* LIGHTSPEED_BASE_ACTIONS_DISPATCHER_H_ */
