#pragma once
#include "../containers/optional.h"
#include "../containers/autoArray.h"
#include "../memory/smallAlloc.h"
#include "../iexception.h"
#include "../constructor.h"
#include "../../mt/timeout.h"
#include "../../mt/sleepingobject.h"
#include "../../mt/exceptions/timeoutException.h"
#include "../../mt/fastlock.h"

namespace LightSpeed {



IRuntimeAlloc *getPromiseAlocator();


///Controls any promise
/** Promise objects are templates and they require knowledge of its internal type. It is impossible
 * to control promises without knowing the type in the compile time. This interface allows limited
 * functions to control any promise object. It can be used by dispatchers to manage and control promises
 * registred inside.
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
	 is in the half-resolved state, when the value is already known,
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
	 * Function sends reject exception to all observers attached to this promise. Canceled
	 * promise is not resolved, so owner of the resolution object still able to resolve promise.
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

///Promise object
/** Promise can be used instead result of asynchronous task in case, that result is not yet known.
 * asynchronous task and you should return Promise object as result.
 *
 * Promise object can be copied. Copying just increases count of references.
 *
 * Promise object can be also destroyed before promise is resolved.
 *
 * You can attach various observers to the promise object, that are called once promise
 * is resolved. You can use getValue() or wait() to read result. These functiio
 *
 * Promises can be stored in map as keys. You can compare two Promise references to find
 * whether both are connected with the same future value.
 *
 * @tparam T defines type of the value that will be stored inside of the promise. You can use
 * any type which has a copy constructor and the assignment operator. You can specify 'void' to
 * declare promise without an internal value. However, you cannot declare a promise with T equal
 * to PException, because this type is used for the rejection.
 * 
 * Promise object defines order operator which allows to easy store promises in a map. Promises
 * are ordered by an address of the internal instances.
 */
template<typename T>
class Promise
	:public ComparableLess<Promise<T> > //< you can compare promises
{

protected:	class Future;
public:

	class IObserver;
	class Resolution;
	class Result;


	///Construct empty promise object
	/** Use this to create variable that will later hold a promise. 
	   You can later use assign operator to set promise. You need to 
	   avoid calling of other methods until promise is set - Violation of this
	   rule causes the application crash */
	Promise() {}

	///Creates the Result object
	/** Result object is intended to act as recipient of result. The task running at the background
	   have to put result of the execution to the object, or call reject anytime an exception happen.
	   Result object will accept just one action and then its instance can be released
	   
	   Function creates new result object and returns it as result. Function always creates
	   new result object destroying the previous one. This always give you assurance, that
	   returned object has not been already resolved.

	   You cannot obtain same result object twice or multiple times from the single Promise. However, the Result object
	   is counted reference, you can make multiple copies and they still represent one result value.	   

	   @return result object.

	   @note Leaving the last reference in the scope causes, that promise is rejected by special exception.
	 */
	   
	    	    
	Result createResult();
	///Creates the result object
	/** Result object is intended to act as recipient of result. The task running at the background
	   have to put result of the execution to the object, or call reject anytime an exception happen.
	   Result object will accept just one action and then its instance can be released

	   Function creates new result object and returns it as result. Function always creates
	   new result object destroying the previous one. This always give you assurance, that
	   returned object has not been already resolved.

	   You cannot obtain same result object twice or multiple times from the single Promise. However, the Result object
	   is counted reference, you can make multiple copies and they still represent one result value.

	   @param reference to allocator that will be used to allocate result object.
	   @return result object.

	   @note Leaving the last reference in the scope causes, that promise is rejected by special exception.
	  */
	Result createResult(IRuntimeAlloc &alloc);

	///Initialized promise but doesn't prepare result object
	/** By default, promise objects are constructed uninitialized due performance reason. You have to create result object
	   to start working with the promise, or assign to the promise another promise, because promises are counted references.

	   This is third way how to create promise without need to work with result object. It is useful when you need to create
	   promise object, add observers and then much much later you will need to create result object.

	   Function init() performs full promise initialization. After initialization, you can attach observers, but they
	   will stay unresolved till somebody creates result object and resolves it.

	   After initialization, you can call createResult(). If the promise is destroyed without result creation, observers
	   are rejected similar to cancel() method.
	 */

	void init();
	void init(IRuntimeAlloc &alloc);


	///Reads value from the promise
	/**
	  Function waits for resolution
	  @param tm specifies timeout for reading.
	  @return result of promise once it is resolved
	  @exception any function throws rejection exception
	 */
	const T &wait(const Timeout &tm) const;

	///Reads value from the promise
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
	 * @return if resolevFn returns Promise<T>, this function returns a promise
	 *  which is resolved once the function also resolves its promise. If
	 *  the resolveFn returns T, this function returns promise, which becomes
	 *  resolved once resolveFn returns and will contain result of that function.
	 *
	 * @note function must return T or Promise<T> or anything compatible with T. 
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
	Promise then(Fn fn);

	///Defines what happens when promise is resolved.
	/**
	 * In compare with standard then(), function's return value is ignored. Function
	 * is just called with result as argument.
	 *
	 * @param fn function to call. Function's return value is ignored, even if 
	 *   it is a Promise
	 *
	 * @return function returns this promise
	 */

	template<typename Fn>
	Promise thenCall(Fn resolveFn);

	///Resolves another promise after current promise is resolved
	/**
	 * @param resolution resolution object that will be used to resolve another promise using
	 * result of current promise
	 *
	 * @return function returns this promise
	 */
	Promise then(const Result &resolution);

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
	Promise then(Fn resolveFn, RFn rejectFn);

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
	 * @see whenRejectedCall
	 */
	template<typename Fn>
	Promise whenRejected(Fn rejectFn);

	///Specifies what happens, when promise is rejected and you don't want to handle return value
	/**
     * @param fn Function to call once promise is rejected. Return value is ignored.
	 */
	template<typename Fn>
	Promise whenRejectedCall(Fn rejectFn);

	///When promise is resolved, then specified sleeping object is wakenup.
	/**
	 * @param sleep sleeping object to wake up
	 * @param resolveReason reason value passed when promise is resolved
	 * @param rejectReason reason value passed when promise is rejected
	 * @return new promise object refers the current promise.
	 *
	 * @note ISleepingObject doesn't carry value of the promise. Woken thread must
	 * read value using getValue()
	 */
	Promise thenWake(ISleepingObject &sleep, natural resolveReason=0, natural rejectReason=1);

	///Registers additional observer 
	/** Registers user defined observer.
	 *
	 * @param ifc pointer to an observer. Observer is identified by its address, so you cannot
	 * register observer twice (but you should not try it. To achieve best performance, this
	 * constrain is not checked in release version)
	 *
	 * Registered observer is not owned by the promise. You have to track ownership by own. But
	 * every promise is always finally resolved, so you can destroy the observer when
	 * promise is resolved without need to track pointer's ownership and aware about memory leaks.
	 *
	 * It is possible to register observer at multiple promises, but in this case, observer
	 * will be called by multiple time without giving it track which promises did resolve.
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
	Promise isolate() ;

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
	static Promise transform(Promise<X> original);

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
	static Promise transform(Promise<X> original, Fn fn);


	///Retrieves promises control interface
	/**
	 * @return interface that allows to control promise. See IPromiseControl description.
	 *
	 */
	PPromiseControl getControlInterface() {return PPromiseControl(future);}


	///Allows to join resolution of two promises into one.
	/**
	 * Function returns promise which resolves once both promises (right and left) resolves. Result
	 * of this resolution is always right side, result of left promise is ignored.
	 *
	 * @param other right side of the operator
	 *
	 * @return promise carrying result of right promise
	 *
	 * @note you can combine various types of promises. Type of right operand is always returned as
	 * a result.
	 *
	 * @note When any of promises is rejected, whole promise is rejected. Note that if left
	 * operand is rejected, result promise can be still considered as unresolved until the second
	 * operand is resolved.
	 */
	template<typename X>
	Promise<X> operator && (const Promise<X> &other) ;

	///Resolves when one of both promises resolved first.

//	Promise operator || (const Promise &other) ;


	///Allows create much flexible observers than classical "then" or "whenRejected"
	/** Observer is object, that receives result or rejection reason once the promise is resolved
	 * You can create observer by implementing this interface. You have to implement methods
	 * resolve() and reject().
	 *
	 * Registered observers are not owned by the promise. You have to track ownership by own.
	 * The most common way is to destroy observer when promise is resolved, because promise
	 * cannot be resolved twice or multiple times.
	 */
	class IObserver {
	public:
		///Promise is resolved by a result
		/**
		 * @param result promise's result.
		 *
		 * Observer doesn't return value because observers are not connected with next Promise.
		 * They are low-level objects. If you need to "return" a value, send the value to
		 * a PromiseResoluton object.
		 *
		 * Function cannot throw exception. If exception happen, observer should store the exception
		 * or resolve next promise by reject()
		 *
		 */
		virtual void resolve(const T &result) throw() = 0;
		///Promise is rejected
		/**
		 * @param e exception caused rejection
		 *
		 * Observer doesn't return value because observers are not connected with next Promise.
		 * They are low-level objects. If you need to "return" a value , send the value to
		 * a PromiseResoluton object.
		 *
		 *
		 * You cannot throw exception, because observers are declared "nothrow". In case of
		 * exception, you should to store the exception object or resolve the next promise by reject()
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
		void resolve(const IConstructor<T> &result) throw();

		///resolve promise by using another promise
		/**
		 * @param result another promise. Function doesn't resolve the promise now
		 *    but promise becomes resolved, once the supplied promise is also resolved
		 */
		void resolve(Promise<T> result);


		///resolve promise by constructing its value directly inside promise
		/**
		 * @param result reference to Constructor template which contains all necessary
		 *    informations to construct the result.
		 *
		 */
		template<typename Impl>
		void resolve(const Constructor<T, Impl> &result) throw();


		virtual ~Resolution() {}

	};

	///Object to store a  result of the promise
	/** storing the result to this object makes promise resolved */
	class Result: public Resolution {
	public:
		///you can clone result object however, it is still the same one result
		Result(const Result &other);

		///destruct the result clone
		/** When last instance of the result is destroyed, promise is resolved with an exception */
		~Result();

		using Resolution::resolve;


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


		PPromiseControl getControlInterface() const {return PPromiseControl(ptr.get());}


		template<typename Fn>
		void callAndResolve(Fn fn) throw();
		template<typename Fn, typename Arg>
		void callAndResolve(Fn fn, Arg arg) throw();

	protected:
		friend class Promise<T>;
		Result(Future *r);

		void cleanup() {
			ptr->releaseResultRef();
			ptr = nil;
		}

		RefCntPtr<Future> ptr;

	private:
		Result &operator=(const Result &other);
	};


protected:

	Promise(Future *p) :future(p) {}

	friend class ComparableLess<Promise<T> >;

	bool lessThan(const Promise<T> &other) const {return future.lessThan(other.future);}
	bool isNil() const {return future.isNil();}

	class Future:public IPromiseControl, public Resolution, public DynObject  {
	public:
		
		Future(IRuntimeAlloc &alloc):alloc(alloc),resultRefCnt(0),resolving(false) {}
		~Future();
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



		FastLock *getLockPtr();
	protected:
		mutable FastLock lock;
		Optional<T> value;
		PException exception;

		typedef AutoArray<IObserver *, SmallAlloc<4> > Observers;
		Observers observers;
		atomic resultRefCnt;

		bool resolving;

		friend class Promise;

		template<typename X, typename Y>
		void resolveInternal( X &var, const Y & result);
		bool isResolved() const throw();
	};


	RefCntPtr<Future> future;

	template<typename X>
	static X transformHelper(const T &value);

	template<typename X> friend class Promise;
};


///Specialization for void argument.
/** Because void argument is a special type, we have specialization for it. Promises with void
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
class Promise<void>: public Promise<Empty> {
public:

	Promise() {}
	Promise(const Promise<Empty> &e):Promise<Empty>(e) {}

	class Result: public Promise<Empty>::Result {
	public:
		typedef Promise<Empty>::Result Super;
		Result(const Super &other):Super(other) {}
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

	template<typename Fn>
	struct EmptyCallVoid {
	public:
		Fn fn;
		EmptyCallVoid(Fn fn):fn(fn) {}
		const Empty &operator()(const Empty &x) const {fn();return x;}

	};

	template<typename Fn>
	Promise then(Fn fn);
	template<typename Fn>
	Promise thenCall(Fn fn);
	template<typename Fn, typename RFn>
	Promise then(Fn resolveFn, RFn rejectFn);

	///attach another promise object to current promise
	/**
	 * another promise is attached and resolved  along with current promise
	 *
	 * @param resolution resolution of another promise
	 * @return this promise
	 *
	 */
	Promise then(const Result &resolution);

	///Transforms any promise into Promise<void>
	/** This can be used to create a notification about resolution without carrying the value.
	 *
	 * @param original original promise
	 * @return new promise of void type
	 */
	template<typename X>
	static Promise transform(Promise<X> original);

	///Blocks thread execution until promise is resolved
	/**
	 * @param tm desired timeout
	 *
	 * @exception TimeoutException timeout elapsed before resolution
	 * @exception any promise has been rejected with an exception.
	 */
	void wait(const Timeout &tm) {Promise<Empty>::wait(tm);}




	Result createResult() {return Promise<Empty>::createResult();}
	Result createResult(IRuntimeAlloc &alloc) {return Promise<Empty>::createResult(alloc);}

};


}

