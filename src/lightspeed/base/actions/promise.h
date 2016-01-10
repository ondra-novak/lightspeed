#pragma once
#include "../containers/optional.h"
#include "../memory/smallAlloc.h"
#include "../iexception.h"
#include "../constructor.h"
#include "../../mt/timeout.h"
#include "../../mt/sleepingobject.h"
#include "../../mt/exceptions/timeoutException.h"
#include "../../mt/fastlock.h"
#include "../containers/deque.h"

namespace LightSpeed {



IRuntimeAlloc *getPromiseAlocator();


///Controls any promise
/** Future/Promise objects are templates and they require knowledge of its internal type. It is impossible
 * to control promises without knowing the type in the compile time. This interface allows limited
 * functions to control any promise object. It can be used by dispatchers to manage and control promises
 * registered inside.
 *
 * Each promise exposes this interface. It can be also used to refer the promises in containers
 */
class IPromiseControl: public RefCntObj {
public:
	virtual ~IPromiseControl() {}

	///State of the promise
	/** In the original idea, promise can have two states. Not resolved
	 and resolved. The idea expects, that observers will be notified
	 instantly and/or asynchronously so resolved promise doesn't care about
	 state of the observers. In the LightSpeed, observers
	 are notified synchronously in the context of the thread that 
	 finally resolved the promise. Because of this, the promise is
	 not resolved instantly, there is always chance that promise 
	 is in a half-resolved state, when the value is already known,
	 but not all observers has been notified.
	 */
	enum State {
		///Promise is not resolved yet and waiting for resolution
		stateNotResolved,
		///Promise is currently resolving
		/** In this state, you cannot resolve the promise, because
		 promise has been already resolved and thread is currently
		 notifies observers. However, promise is not
		 considered as resolved yet, so new observers bound
		 to the promise is not immediately notified, they are just
		 enqueued to be notified later once the thread finishes of the notification
		 of the observers registered previously */
		stateResolving,
		///Promise has been resolved
		/** Observer added to the promise is immediately notified
		 in the context of the current thread */
		stateResolved
	};

	///Cancels the promise
	/**
	 * Function sends reject exception to all observers attached to this Promise. Canceled
	 * Promise is not resolved, so owner of the Promise object still able to resolve promise.
	 *
	 * Dispatchers can use function to cancel all registered promises if they need to empty their
	 * collectors
	 *
	 * @param pointer to exception to use for cancellation
	 * @retval stateNotResolved Promise was not resolved, all observers has been removed and
	 *   thus they will not executed. Function was able to cancel all observers. 
	 * @retval stateResolving Promise was in resolving state, so some observers has already 
		 received the value and some observers are already queued for notification - they
		 cannot be canceled. Function was unsuccessful, program need to wait until
		 promise resolves complete.
	   @retval stateResolved Promise was already resolved, so function can no longer 
	    cancel anything.

	   @note Function synchronously executes whenRejected() observers.
	 */
	virtual State cancel(const PException &e) throw () = 0;

	///Cancels the promise
	/**
	 * Function sends reject exception to all observers attached to this promise. Canceled
	 * promise is not resolved, so owner of the resolution object still able to resolve promise.
	 *
	 * Dispatchers can use function to cancel all registered promises if they need to empty their
	 * collectors
	 *
	 * @retval stateNotResolved Promise was not resolved, all observers has been removed and
	 *   thus they will not executed. Function was able to cancel all observers.
	 * @retval stateResolving Promise was in resolving state, so some observers has already
	 received the value and some observers are already queued for notification - they
	 cannot be canceled. Function was unsuccessful, program need to wait until
	 promise resolves complete.
	 @retval stateResolved Promise was already resolved, so function can no longer
	 cancel anything.

	 @note Function synchronously executes whenRejected() observers.
	 * @note function sends CanceledException to all observers
	 */
	virtual State cancel() throw () = 0;

	///determines resolution state
	/**
	 * You can use this function to determine whether promise has been resolved and then
	 * you can for example stop tracking it.
	 *
	 * @retval true promise is resolved or rejected.
	 * @retval false promise is still waiting for resolution
	 *
	 * @note To achieve backward compatibility, function returns true also when the promise is
	 * in "resolving" state.
	 */
	virtual bool resolved() const throw () = 0;

	///Retrieves the state of the promise
	/**
	 @retval the state of the promise
	 @note if function is called from other thread, result can be inaccurate, because
	  state can change anytime later. If called in the observer, function returns 
	  stateResolving. The only stable state is stateResolved
	 */
	virtual State getState() const throw () = 0;

	///Wait for promise resolution	
	/** Function cannot determine result of the promise. It can just only wait for the promise resolution */
	virtual void wait(const Timeout &tm) const = 0;

};

typedef RefCntPtr<IPromiseControl> PPromiseControl;

template<typename T> class Promise;

///Future object
/**
  Future is read-only variable which will contain value in a near future. Variable as always two parts.
  First part is Future itself, second part is called Promise. Value assigned to the promise becomes
  available on future. You can pass Promise into function which running in background thread and use the
  Promise to store result of the function. The result then becomes available through the Future.

  To constructuct Future/Promise, you have to create Future first. Then you can call createPromise() to create
  Promise from this future and pass the variable to a function or a background task  

  Futures and Promises are inspired by Promise object from JavaScript. Future variable can be used to
  install various observers or make chain of processing before the final value is observed.

  To define future processing, use function then() which accept a function which will be called once the promise
  is resolved. Futures can be chained through the function then(), because every then() returns new Future variable.
  
  @tparam T contains type of the value. You can also specify 'void' to create future which only holds resolution
  state without a value. You cannot create Future for PException, because this type carries a possible exception through the
  future/promise chain.

  Pass promise to a asynchronous function and wait for result
  @code
    Future<int> future;
	runAsyncFoo(arg, future.createPromise())
	int v = future;
  @endcode

  Pass promise to a asynchronous function and define future processing of the result. Once
  chain is created, the thread no longer need to keep Future variable and don't need to wait to the
  future. Once promise is resolved, result carried through the chain
   @code 
	  Future<int> future;
	  runAsyncFoo(arg, future.createPromise())
	  future.then(processResult1).then(useResult)
  @endcode

  You can define own observer. Once promise is resolved, the observer is notified with a value
  @code
     Future<int> future;
	 runAsyncFoo(arg, future.createPromise())
	 future.addObserver(new ValueObserver(...) )
  @endcode


  @note Future instance works as reference. You can copy the variable, however it still refers the same
  value. 

  @note Instance of the Future can destroyed anytime even if the Promise not yet been resulved. If there
  is no other reference and no observer, resolving such promise doesn't create a problem. Thread that
  resolving the promise doesn't need to take care about the Future
 */
template<typename T>
class Future
	:public ComparableLess<Future<T> > //< you can compare promises
{

protected:	
	class Value;
public:

	class IObserver;
	class Resolution;
	friend class Promise<T>;


	///Construct empty future object
	/** 
	 @note Function doesn't create internal value. It will be create later as needed.
	 */
	Future() {}
	///Construct future object
	/** Function uses allocator to allocate internal structure
	*/
	Future(IRuntimeAlloc &alloc);
	///Returns promise
	/** 
	 The Promise is implemented as reference. You can have multiple promises but all of them refers 
	 the same future
	 */

	Promise<T> getPromise();

	///Clears the future
	/** By clearing the varuable you can create new future. This new future doesn't affect any future/promise
	create by prevous instance. Function just clears this reference */
	void clear();

	///Clears the future and create new future using different allocator
	/**
	@param alloc allocator to allocate future internals
	*/
	void clear(IRuntimeAlloc &alloc);

	///Reads value from the future
	/**
	  Function waits for resolution
	  @param tm specifies timeout for reading.
	  @return result of promise once it is resolved
	  @exception any function throws rejection exception
	 */
	const T &wait(const Timeout &tm) const;

	///Reads value from the future
	/**
	  Function waits for resolution. Function has no timeout, so it will wait infinitively to resolution
	  @return result of promise once it is resolved
	  @exception any function throws rejection exception
	 */
	const T &wait() const;

	///Alias to wait. Function retrieves value of the promise if resolved, otherwise blocks.
	const T &getValue() const {return wait();}

	///Reads value - same as wait(), just with infinite timeout
	operator const T &() const {return wait();}

	///Define what happens when promise is resolved
	/**
     * @param fn Function to call once promise is resolved
	 * @return if resolevFn returns Future<T>, this function returns a promise
	 *  which is resolved once the function also resolves its promise. If
	 *  the resolveFn returns T, this function returns promise, which becomes
	 *  resolved once resolveFn returns and will contain result of that function.
	 *
	 * @note function must return T or Future<T> or PException. If Future is returned
	 * the promise is resolved once returned Future receives the value. If PException
	 * is returned, promise is resolved with exception
	 *
	 * If you need just call any function and you don't care about return value,
	 * use thenCall instead. Function thenCall is also faster, because it doesn't
	 * need to create Promise
	 *
	 * @note - Technical note - Functions are called in the context of thread
	 *  doing resolution. If you need to call function in another context, you
	 *  have to execute function through the IExecutor. 
	 *
	 * @see thenCall
	 */
	 
	template<typename Fn>
	Future then(Fn fn);

	///Defines what happens when promise is resolved.
	/**
	 * In compare with standard then(), function's return value is ignored. Function
	 * is just called with result as argument.
	 *
	 * @param fn function to call. Function's return value is ignored, even if 
	 *   it is a Future
	 *
	 * @return function returns this promise
	 */

	template<typename Fn>
	Future thenCall(Fn resolveFn);

	///Resolves another promise after the current promise is resolved
	/**
	 * @param promise resolution object that will be used to resolve another promise using
	 * result of current promise
	 *
	 * @return function returns this promise
	 */
	Future then(const Promise<T> &promise);

	///Define what happens when promise is resolved
	/**
     * @param resolveFn Function to call once promise is resolved
	 * @return if resolevFn returns Promise<T>, this function returns a promise
	 *  which is resolved once the function also resolves its promise. If
	 *  the resolveFn returns T, this function returns promise, which becomes
	 *  resolved once resolveFn returns and will contain result of that function.
	 *  If resolveFn returns anything other, this function returns 
	 *  copy of this promise, which resolves after the resolveFn returns with
	 * the original result.
	 *
	 * @param rejectFn Function to call once promise is rejected. There are 
	 * similar rules as for resolveFn
	 *
	 * @note - Technical note - Functions are called in the context of thread
	 *  doing resolution. If you need to call function in another context, you
	 *  have to execute function through the IExecutor. 
	 */
	template<typename Fn, typename RFn>
	Future then(Fn resolveFn, RFn rejectFn);

	///Specifies what happens, when promise is rejected
	/**
     * @param fn Function to call once promise is rejected
	 * @return if resolevFn returns Promise<T>, this function returns a promise
	 *  which is resolved once the function also resolves its promise. If
	 *  the resolveFn returns PException, this function returns promise, which becomes
	 *  rejected once rejectFn returns and will contain exception.
	 *
	 * @note function must return PException or Promise<T>
	 * If you need just call any function and you don't care about return value,
	 * use whenRejectedCall instead. Function whenRejectedCall is also faster, because it doesn't
	 * need to create Promise
	 *
	 * Function can return Promise of <T> which means that on rejection, function
	 * can handle the rejection and perform another action to resolve the promise.
	 * In case of success, function resolves the promise and execution continues
	 * like the original promise was resolved. Function also can reject the promise
	 * and execution continues by next rejection handler. Regardless on
	 * what returned, original promise is always rejected
	 *
	 * @note - Technical note - Functions are called in the context of thread
	 *  doing resolution. If you need to call function in another context, you
	 *  have to execute function through the IExecutor. 
	 *
	 * @see onException
	 */
	template<typename Fn>
	Future onException(Fn rejectFn);

	///Specifies what happens, when promise is rejected and you don't want to handle return value
	/**
     * @param fn Function to call once promise is rejected. Return value is ignored.
	 */
	template<typename Fn>
	Future onExceptionCall(Fn rejectFn);

	///When promise is resolved, then specified sleeping object is waken up.
	/**
	 * @param sleep sleeping object to wake up
	 * @param resolveReason reason value passed when promise is resolved
	 * @param rejectReason reason value passed when promise is rejected
	 * @return new promise object refers the current promise.
	 *
	 * @note ISleepingObject doesn't carry value of the promise. Woken thread must
	 * read value using getValue()
	 */
	Future thenWake(ISleepingObject &sleep, natural resolveReason=0, natural rejectReason=1);

	///Registers additional observer 
	/** Registers user defined observer.
	 *
	 * @param ifc pointer to an observer. Observer is identified by its address, so you cannot
	 * register observer twice (but you should not try it. To achieve best performance, this
	 * constrain is not checked in release version)
	 *
	 * Registered observer is not owned by the promise. You have to track ownership by own. But
	 * every promise is always finally resolved, so you can destroy the observer when
	 * promise is resolved without need to track pointer's ownership and afraid about memory leaks.
	 *
	 * It is strongly recommended to create new instance for every promise. Never share the observer between promises
	 *
	 * @code
	 * class MyObserver: public Future<int>::IObserver {
	 * public:
	 *  virtual void resolve(const int &result) throw() {
	 *    //...do something with result...
	 *    delete this;
	 *  }
	 *  virtual void resolve(const PException &e) throw() {
	 *    //... do something with exception
	 *     delete this;
	 *  }
	 * };
	 * Promise<int> p;
	 * p.addObserver(new MyObserver);
	 * @endcode
	 *
	 * @note if promise is already resolved in time of registration, observer is immediately
	 * executed in the content of the current thread and before function returns. If observer
	 * destroys itself with resolution then the pointer no longer points to a valid object
	 *
	 */
	void addObserver(IObserver *ifc);
	///Removes observer
	/** Allows you to remove observer before the resolution
	 *
	 * @param ifc pointer to observer to remove
	 * @retval true observer has been found and removed. So you can now work with the observer,
	 * eventually destroy it
	 * @retval false observer did not found. If promise is already resolved, observer was
	 * probably destroyed
	 *
	 * @note pointer is not dereferenced. You can use address of already destroyed observer.
	 *
	 */
	bool removeObserver(IObserver *ifc);

	IPromiseControl::State getState() const throw ();

	///Cancels the promise
	/**
	 * Function sends reject exception to all observers attached to this promise. Canceled
	 * promise is not resolved, so owner of the resolution object still able to resolve promise. This
	 * also mean, that you are still able to attach observer after cancellation. Function only
	 * affects all currently registered observers.
	 *
	 * @param exception exception to use for cancellation.
	 */
	IPromiseControl::State cancel(const Exception &exception) throw();

	///Cancels the promise
	/**
	 * Function sends reject exception to all observers attached to this promise. Canceled
	 * promise is not resolved, so owner of the resolution object still able to resolve promise. This
	 * also mean, that you are still able to attach observer after cancellation. Function only
	 * affects all currently registered observers.
	 *
	 * @param pointer to exception to use for cancellation
	 */
	IPromiseControl::State cancel(PException exception) throw();


	///Cancels the promise
	/**
	 * Function sends reject exception to all observers attached to this promise. Canceled
	 * promise is not resolved, so owner of the resolution object still able to resolve promise. This
	 * also mean, that you are still able to attach observer after cancellation. Function only
	 * affects all currently registered observers.
	 *
	 * @note function send CanceledException to all subscribers
	 */
	IPromiseControl::State cancel() throw();


	///Isolates this promise object
	/**
	 * By default, promise object shares all internals across all instances. This function
	 * isolates the promise and returns new object, which has separated state. Isolated
	 * promise is still resolved, when original promise is resolved, but you can attach
	 * own observers and use the cancel() to remove them. This is particular useful for queues
	 * or dispatches to handle destruction of such objects, when the promise must be canceled
	 * because the queue or the dispatcher is going to the destruction.
	 *
	 * @return new isolated promise
	 *
	 * @note function just chains promise 1:1. It has same effect as attach then() which contains
	 * empty function.
	 */
	Future isolate() ;

	///Transform type of promise to another type
	/**
	 * Function is static so you have to call it using:
	 * @code
	 * Promise<Y> promise_y = Promise<Y>::transform(promise_x)
	 * @endcode
	 * ... where promise_x is original promise and Y is a new type. There must be automatic
	 * conversion available through the constructor or conversion operator. Function can also
	 * handle conversion through explicit constructor.
	 *
	 * @param original original promise
	 * @return new promise
	 */
	template<typename X>
	static Future transform(Future<X> original);

	///Transform type of promise to another type
	/**
	 * Function is static so you have to call it using:
	 * @code
	 * Promise<Y> promise_y = Promise<Y>::transform(promise_x,&tranform_fn);
	 * @endcode
	 * ... where promise_x is original promise and Y is a new type. Function will use
	 * function to convert value to the new type.
	 *
	 * @param original original promise
	 * @param function that handles conversion.
	 * @return new promise
	 */
	template<typename X, typename Fn>
	static Future transform(Future<X> original, Fn fn);


	///Retrieves promises control interface
	/**
	 * @return interface that allows to control promise. See IPromiseControl description.
	 *
	 */
	PPromiseControl getControlInterface();


	///Allows create much flexible observers than classical "then" or "onException"
	/** Observer is object, that receives result or rejection reason once the promise is resolved
	 * You can create observer by implementing this interface. You have to implement methods
	 * resolve() and reject().
	 *
	 * Registered observers are not owned by the promise. You have to track ownership by own.
	 * The most common way is to destroy observer when promise is resolved, because promise
	 * cannot be resolved twice or left unresolved
	 */
	class IObserver {
	public:
		///Promise is resolved by a result
		/**
		 * @param result promise's result.
		 *
		 * Observer doesn't return value because observers are not connected with next Promise.
		 * They are low-level objects. If you need to "return" a value, send the value to
		 * an another Promise object.
		 *
		 * Function cannot throw exception. If exception happen, observer should store the exception
		 * or resolve next promise with exception
		 *
		 */
		virtual void resolve(const T &result) throw() = 0;
		///Promise is resolved with exception
		/**
		 * @param e exception caused rejection
		 *
		 * Observer doesn't return value because observers are not connected with next Promise.
		 * They are low-level objects. If you need to "return" a value , send the value to
		 * an another Promise object.
		 *
		 *
		 * You cannot throw exception, because observers are declared "nothrow". In case of
		 * exception, you should to store the exception object or resolve the next promise with exception
		 */
		virtual void resolve(const PException &e) throw()= 0;

		virtual ~IObserver() {}
	};

	///Interface to resolve promise
	class Resolution: public IObserver {
	public:

		using IObserver::resolve;
		///resolve promise by constructing its value directly inside promise
		/**
		 * @param result reference to IConstructor which contains all necessary
		 *    informations to construct the result
		 *
		 */
		virtual void resolve(const IConstructor<T> &result) throw();

		///resolve promise by using another promise
		/**
		 * @param result another promise. Function doesn't resolve the promise now
		 *    but promise becomes resolved, once the supplied promise is also resolved
		 */
		void resolve(Future<T> result);


		///resolve promise by constructing its value directly inside promise
		/**
		 * @param result reference to Constructor template which contains all necessary
		 *    informations to construct the result.
		 *
		 */
		template<typename Impl>
		void resolve(const Constructor<T, Impl> &result) throw();

		virtual Value *getValue() = 0;

		virtual ~Resolution() {}

	};


protected:

	Future(Value *v) :future(v) {}

	void init();



	friend class ComparableLess<Future<T> >;

	bool lessThan(const Future<T> &other) const {return future < other.future;}
	bool isNil() const {return future != nil;}

	class Value:public IPromiseControl, public Resolution, public DynObject  {
	public:
		
		Value(IRuntimeAlloc &alloc):alloc(alloc),resultRefCnt(0),resolving(false) {}
		~Value();
		virtual bool resolved() const throw ();
		virtual State getState() const throw ();
		virtual void resolve(const T &result) throw() ;
		virtual void resolve(const IConstructor<T> &result) throw ();
		virtual void resolve(const PException &e) throw();
		void registerObserver(IObserver *ifc);
		bool unregisterObserver(IObserver *ifc);
		virtual State cancel(const PException &e) throw();
		virtual State cancel() throw ();
		IRuntimeAlloc &alloc;
		void addResultRef();
		void releaseResultRef();

		virtual void wait(const Timeout &tm) const;
		virtual Value *getValue() { return this; }



	protected:
		mutable FastLock lock;
		Optional<T> value;
		PException exception;

		typedef Deque<IObserver *, SmallAlloc<4> > Observers;
		Observers observers;
		atomic resultRefCnt;

		bool resolving;

		friend class Future;
		friend class Promise<T>;

		template<typename X, typename Y>
		void resolveInternal( X &var, const Y & result);
		bool isResolved() const throw();
	};


	RefCntPtr<Value> future;

	template<typename X>
	static X transformHelper(const T &value);

	template<typename X> friend class Future;

	Value *getValuePtr() const  {
		if (future == nil) const_cast<Future *>(this)->init();
		return future;
	}


};


///Specialization for void argument.
/** Because void argument is a special type, we have specialization for it. Futures with void
 * type should accept observer functions without arguments and they should not expect return value of the
 * observer function. Also resolve promise should be called without arguments.
 *
 * Specialization uses Promise<Empty> where Empty is special empty class. During lifetime of
 * the promise, the Empty instance is carried through, but it should not leave the promise object out.
 *
 * Functions then() and thenCall() accepts observers without argument. Conversion to observer with
 * "the Empty" argument is done by the internal function EmptyCallVoid. This function shallows
 * the observer function and calls it without argument. It also doesn't expect return value.
 *
 * To resolve Promise<void> you should construct PromiseResolution<void>. This object
 * contains simple function resolve() without argument. Function internally constructs Empty instance
 * and uses it to process whole observer chain.
 *
 * Promise<void> can be anytime converted to Promise<Empty> and vice versa.
 *
 *
 * It is possible to receive value of the promise, but you will receive an instance of the Empty class
 *
 * Function wait() doesn't return value, but still can throw an exception in case, that promise
 * is rejected.
 *
 */
template<>
class Future<void>: public Future<Empty> {
public:

	Future() {}
	Future(const Future<Empty> &e):Future<Empty>(e) {}


	template<typename Fn>
	struct EmptyCallVoid {
	public:
		Fn fn;
		EmptyCallVoid(Fn fn):fn(fn) {}
		const Empty &operator()(const Empty &x) const {fn();return x;}

	};

	class IObserver : public Future<Empty>::IObserver {
	public:
		virtual void resolve(const Empty &) throw() {
			resolve();
		}
		virtual void resolve() throw() = 0;
		virtual void resolve(const PException &e) throw() = 0;		
	};

	template<typename Fn>
	Future then(Fn fn);
	template<typename Fn>
	Future thenCall(Fn fn);
	template<typename Fn, typename RFn>
	Future then(Fn resolveFn, RFn rejectFn);

	///attach another promise object to current promise
	/**
	 * another promise is attached and resolved  along with current promise
	 *
	 * @param resolution resolution of another promise
	 * @return this promise
	 *
	 */
	Future then(const Promise<void> &promise);

	///Transforms any promise into Promise<void>
	/** This can be used to create a notification about resolution without carrying the value.
	 *
	 * @param original original promise
	 * @return new promise of void type
	 */
	template<typename X>
	static Future transform(Future<X> original);

	///Blocks thread execution until promise is resolved
	/**
	 * @param tm desired timeout
	 *
	 * @exception TimeoutException timeout elapsed before resolution
	 * @exception any promise has been rejected with an exception.
	 */
	void wait(const Timeout &tm) {Future<Empty>::wait(tm);}

	Promise<void> getPromise();

};


///Object to store a  result of the future
/** storing the result to this object makes promise resolved

Variable declared as Promise is implemented as reference. You can copy content of variable,
but this only copies reference to the same promise. You can also destroy  Promise without
resolving. As long as there is at least once reference, promise stays unresolved. Once last
reference is removed, internal promise is resolved with the exception CanceledException.

*/
template<typename T>
class Promise : public Future<T>::Resolution {
public:
	///you can clone result object however, it is still the same one result
	Promise(const Promise &other);

	Promise(const Future<T> &future);

	///destruct the result clone
	/** When last instance of the result is destroyed, promise is resolved with an exception */
	~Promise();

	using  Future<T>::Resolution::resolve;


	virtual void resolve(const T &result) throw() {
		if (ptr != nil) ptr->resolve(result);
	}
	virtual void resolve(const IConstructor<T> &result) throw() {
		if (ptr != nil) ptr->resolve(result);
	}
	virtual void resolve(const PException &e) throw() {
		if (ptr != nil) ptr->resolve(e);
	}

	///Rejects this promise
	/** You should reject the promise using resolve with an exception as argument. However
	this can cause compiler confusion and generate errors about ambiguity. Function reject()
	make rejection easier. Just pass exception object directly */
	void reject(const Exception &e) throw() {
		PException ce = e.clone();
		resolve(ce.getMT());
	}
	///Rejects this promise
	/** You should reject the promise using resolve with an exception as argument. However
	this can cause compiler confusion and generate errors about ambiguity. Function reject()
	make rejection easier. Just pass exception object directly */
	void reject(const PException &e) throw() {
		resolve(e.getMT());
	}

	///setting value to the promise also resolves the promise
	const T &operator=(const T & v) {
		resolve(v);
		return v;
	}

	///You can receive reference to a control object from the promise
	/**
	@return reference to control object. Note that function returns exactly same pointer as Future. You
	can use returned value to compare Future an Promise to determine, whether promise matches to the future.
	*/
	PPromiseControl getControlInterface() const { return PPromiseControl(ptr.get()); }


	///Function calls another function and uses return value to resolve promise
	/**
	@param fn function to call. Function can return direct value, future value or PException. This affect how
	promise will be resolved
	*/
	template<typename Fn>
	void callAndResolve(Fn fn) throw();
	///Function calls another function and uses return value to resolve promise
	/**
	@param fn function to call. Function can return direct value, future value or PException. This affect how
	promise will be resolved
	@param arg argument for the function
	*/
	template<typename Fn, typename Arg>
	void callAndResolve(Fn fn, Arg arg) throw();

protected:
	friend class Future<T>;
	Promise(typename Future<T>::Value *r);

	void cleanup() {
		ptr->releaseResultRef();
		ptr = nil;
	}

	virtual typename Future<T>::Value *getValue() { return ptr; }

	RefCntPtr<typename Future<T>::Value> ptr;
};


template<>
class Promise<void> : public Promise<Empty> {
public:
	typedef Promise<Empty> Super;
	Promise(const Super &other) :Super(other) {}
	Promise(const Future<void> &future) : Super(future) {}
	using Super::resolve;
	virtual void resolve() throw() {
		Empty x;
		Super::resolve(x);
	}

	template<typename Fn>
	void callAndResolve(Fn fn) throw();
	template<typename Fn, typename Arg>
	void callAndResolve(Fn fn, Arg arg) throw();

};

}

