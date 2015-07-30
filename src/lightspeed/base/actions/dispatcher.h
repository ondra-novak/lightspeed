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


///Interface that allows to give object commands through actions
/**
 * Every object that implements IDispatcher can receive commands through objects packed into
 * Action. The interface should be used with executors and queue dispatchers.
 */
class AbstractDispatcher {
public:

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
	 * @param action action to dispatch. Function can create copy of the function. To create
	 * function object, you can use Action::create. You can also use Action instance with a function
	 * but called function should always create copy.
	 *
	 * @note function distpatch must be MT safe!
	 *
	 */
	virtual void dispatch(const IDispatchAction &action) = 0;

	///Dispatches function call and returns value as promise
	/**
	 * This allows generally dispatch any function to the dispatcher
	 *
	 * @param fn function to dispatch
	 * @param template_promise promise object that can be constructred directly inside the argument.
	 *   It is used to determine type of result promise. This allows convert result of the function
	 *   to another convertible type - for example: from child to parent type, or from integer
	 *   to float, etc. This also allows to compile code under pre C++11 compiler. Value of the
	 *   argument is not used and not changed.
	 * @return
	 *
	 * @code
	 * Promise<int> res = dispatcher.dispatch(&foo, Promise<int>());
	 * dispatcher.dispatch(Promise<float>(),&bar).then(&bas);
	 * @endcode
	 */
	template<typename T, typename Fn>
	Promise<T> dispatch(const Promise<T> &template_promise, Fn fn);

	///Dispatches promise through the dispatcher
	/** Once promise is resolved or rejected, result is carried through the dispatcher.
	 * Function returns new promise, which becomes resolved once the dispatcher processes the result.
	 * This function can be used for example to forward promises to the main dispatcher to process result
	 * in the main thread instead of the thread which resolved the promise.
	 *
	 * @param promise promise to dispatch
	 * @return new promise which becomes resolved once the dispatcher finishes its forward. Callback
	 * functions are called in the context of dispatcher's thread
	 *
	 * @code
	 * Promise<int> x = doAsyncCode();
	 * dispatcher.dispatch(x).then(&onResult,&onError);
	 * @endcode
	 *
	 * Code above receives promise from an asynchronous code execution and dispatches the promise
	 * throught the dispatcher. Once doAsyncCode finishes, result is dispatched and function onResult
	 * is executed in the context of the dispatcher
	 */
	template<typename T>
	Promise<T> dispatch(Promise<T> promise) ;




	virtual ~AbstractDispatcher() {}

protected:
	template<typename T> class PromiseDispatch;

};


}



#endif /* LIGHTSPEED_BASE_ACTIONS_DISPATCHER_H_ */
