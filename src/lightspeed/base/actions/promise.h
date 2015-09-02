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


template<typename T> class Promise;
template<typename T> class PromiseResolution;



IRuntimeAlloc *getPromiseAlocator();


///Controls any promise
/** Promise objects are templates and require knowledge of its internal type. It is impossible
 * to control promises without knowing the type in the compile time. This interface allows limited
 * functions to control any promise object. It can be used by dispatchers to manage and control promises
 * registred inside.
 *
 * Each promise exposes this interface. It can be also used to refer the promises in containers
 */
class IPromiseControl: public RefCntObj {
public:
	virtual ~IPromiseControl() {}

	///Cancels the promise
	/**
	 * Function sends reject exception to all callbacks attached to this promise. Canceled
	 * promise is not resolved, so owner of the resolution object still able to resolve promise.
	 *
	 * Dispatchers can use function to cancel all registered promises if they need to empty their
	 * collectors
	 *
	 * @param pointer to exception to use for cancelation
	 */
	virtual void cancel(const PException &e) throw () = 0;

	///Cancels the promise
	/**
	 * Function sends reject exception to all callbacks attached to this promise. Canceled
	 * promise is not resolved, so owner of the resolution object still able to resolve promise.
	 *
	 * Dispatchers can use function to cancel all registered promises if they need to empty their
	 * collectors
	 *
	 * @note function sends CanceledException to all subscribers
	 */
	virtual void cancel() throw () = 0;
};

typedef RefCntPtr<IPromiseControl> PPromiseControl;

///Promise object
/** Promise can be used instead result of asynchronous task in case, that result is not yet known.
 * You can construct Promise object from PromiseResolution. You have to pass PromiseResolution to the
 * asynchronous task and you should return Promise object as result.
 *
 * Promise object can be copied. Copying just increases count of references.
 *
 * Promise object can be also destroyed before promise is resolved.
 *
 * You can attach various callbacks to the promise object, that are called once promise
 * is resolved. You can use getValue() or wait() to read result. These functiio
 *
 * Promises can be stored in map as keys. You can compare two Promise references to find
 * whether both are connected with the same future value.
 *
 */
template<typename T>
class Promise
	:public ComparableLess<Promise<T> > //< you can compare promises
{
public:

	class Resolution;

	///Construct empty promise object
	/** Use this to create variable that will later hold a promise. 
	   You can later use assign operator to set promise. You need to 
	   avoid calling of other methods until promise is set - Violation of this
	   rule causes application crash */
	Promise() {}

	Promise(PromiseResolution<T> &resolution);
	Promise(PromiseResolution<T> &resolution, IRuntimeAlloc &alloc);


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
	Promise then(const PromiseResolution<T> &resolution);

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
	 * like the orginal promise was resolved. Function also can reject the promise
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

	///Registers additional callback 
	/** This allows to handle resolution by specific way */
	Promise registerCb(Resolution *ifc);
	///Unregisters callback
	/** This allows to handle resolution by specific way */
	Promise unregisterCb(Resolution *ifc);

	///Cancels the promise
	/**
	 * Function sends reject exception to all callbacks attached to this promise. Canceled
	 * promise is not resolved, so owner of the resolution object still able to resolve promise. This
	 * also mean, that you are still able to attach callback after cancelation. Function only
	 * affects all currently registered callbacks.
	 *
	 * @param exception exception to use for cancelation.
	 */
	void cancel(const Exception &exception) throw();

	///Cancels the promise
	/**
	 * Function sends reject exception to all callbacks attached to this promise. Canceled
	 * promise is not resolved, so owner of the resolution object still able to resolve promise. This
	 * also mean, that you are still able to attach callback after cancelation. Function only
	 * affects all currently registered callbacks.
	 *
	 * @param pointer to exception to use for cancelation
	 */
	void cancel(PException exception) throw();


	///Cancels the promise
	/**
	 * Function sends reject exception to all callbacks attached to this promise. Canceled
	 * promise is not resolved, so owner of the resolution object still able to resolve promise. This
	 * also mean, that you are still able to attach callback after cancelation. Function only
	 * affects all currently registered callbacks.
	 *
	 * @note function send CanceledException to all subscribers
	 */
	void cancel() throw();


	///Isolates this promise object
	/**
	 * By default, promise object shares all internals accross all instances. This function
	 * isolates the promise and returns new object, which has separated status. Isolated
	 * promise is still resolved, when original promise is resolved, but you can attach
	 * own callbacks and use the cancel() to remove them. This is particular useful for queues
	 * or dispatches to handle destruction of such objects, when the promise must be canceled
	 * because the queue or the dispatcher is going to destruction.
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
	 * Promise<Y> promise_y = Promise<Y>::trans`
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
	 * of this resultion is always right side, result of left promise is ignored.
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

	///Interface to resolve promise
	/** Asynchronous task will receive this interface to resolve promise once
	  the task is finished and result is known

	  Expectants must implement this interface to receive result. There are
	  couple classes that will help with it
	 */
	class Resolution {
	public:

		///resolve promise using a new value
		/**
		 * Resolves promise.
		 * @param result value used to resolve promise
		 *
		 * @note once promise is resolved, this object is destroyed and pointer to it
		 *    becomes invalid. Do not try to resolve the promise multiple times. Any
		 *    additional attempt can crash your program
		 */
		virtual void resolve(const T &result) throw() = 0;
		///resolve promise by constructing its value directly inside promise
		/**
		 * @param result reference to IConstructor which contains all necessary
		 *    informations to construct the result
		 *
		 * @note once promise is resolved, this object is destroyed and pointer to it
		 *    becomes invalid. Do not try to resolve the promise multiple times. Any
		 *    additional attempt can crash your program
		 */
		virtual void resolve(const IConstructor<T> &result) throw();

		///resolve promise by using another promise
		/**
		 * @param result another promise. Function doesn't resolve the promise now
		 *    but promise becomes resolved, once the supplied promise is also resolved
		 *
		 * @note once promise is resolved, this object is destroyed and pointer to it
		 *    becomes invalid. Do not try to resolve the promise multiple times. Any
		 *    additional attempt can crash your program
		 */
		void resolve(Promise<T> result);


		///resolve promise by constructing its value directly inside promise
		/**
		 * @param result reference to Constructor template which contains all necessary
		 *    informations to construct the result.
		 *
		 * @note once promise is resolved, this object is destroyed and pointer to it
		 *    becomes invalid. Do not try to resolve the promise multiple times. Any
		 *    additional attempt can crash your program
		 */
		template<typename Impl>
		void resolve(const Constructor<T, Impl> &result) throw();

		///reject promise by an exception
		/**
		 * @param e exception used to reject promise. Function makes copy of the exception
		 *           and stores the copy inside the promise object.
		 *
		 * @note once promise is resolved, this object is destroyed and pointer to it
		 *    becomes invalid. Do not try to resolve the promise multiple times. Any
		 *    additional attempt can crash your program
		 */

		virtual void reject(const Exception &e) throw();
		///reject promise by an exception
		/**
		 * @param e exception used to reject promise. Function shares exception object
		 *
		 * @note once promise is resolved, this object is destroyed and pointer to it
		 *    becomes invalid. Do not try to resolve the promise multiple times. Any
		 *    additional attempt can crash your program
		 */
		virtual void reject(const PException &e) throw()= 0;
		///reject promise by using another promise
		/**
		 * @param p promise that will resolve this promise in the future
		 *
		 * Actually function works same way as resolve, but this is useful for
		 *  functions passed to the whenRejected that returns promise. When original
		 *  promise is rejected, callback function still can supply another promise, which
		 *  can be resolved and effectively stop rejection chain. This allows to
		 *  implement an alternate way how to resolve promise depend on exception of
		 *  the previous attempt
		 *
		 * @note once promise is resolved, this object is destroyed and pointer to it
		 *    becomes invalid. Do not try to resolve the promise multiple times. Any
		 *    additional attempt can crash your program
		 */
		void reject(Promise<T> p);

		virtual ~Resolution() {}

	};




protected:
	friend class PromiseResolution<T>;
	friend class ComparableLess<Promise<T> >;

	bool lessThan(const Promise<T> &other) const {return future.lessThan(other.future);}
	bool isNil() const {return future.isNil();}

	class Future:public IPromiseControl, public Resolution, public DynObject  {
	public:
		
		Future(IRuntimeAlloc &alloc):alloc(alloc) {}
		bool checkResolved();
		virtual void resolve(const T &result) throw() ;
		virtual void resolve(const IConstructor<T> &result) throw ();
		virtual void reject(const PException &e) throw();
		void registerExpectant(Resolution *ifc);
		void unregisterExpectant(Resolution *ifc);
		virtual void cancel(const PException &e) throw();
		virtual void cancel() throw ();
		IRuntimeAlloc &alloc;


		FastLock *getLockPtr();
	protected:
		FastLock lock;
		Optional<T> value;
		PException exception;

		typedef AutoArray<Resolution *, SmallAlloc<4> > Sleepers;
		Sleepers sleepers;


		template<typename X>
		void resolveInternal( const X & result );
	};

	RefCntPtr<Future> future;

	template<typename X>
	static X transformHelper(const T &value);

	template<typename X> friend class Promise;
};

///Represents result of an asynchronous task. It should be passed to the code that process the task
/**
 * You need the PromiseResolution object to construct promise object. PromiseResolution can be
 * copied similar to Promise. After copying the copy still connected with original promise.
 */
template<class T>
class PromiseResolution: public SharedResource, public Promise<T>::Resolution {
public:

	typedef typename Promise<T>::Resolution Resolution;
	typedef typename Promise<T>::Future Future;

	PromiseResolution():ptr(0) {}
	PromiseResolution(const PromiseResolution &other):SharedResource(other,other.ptr?other.ptr->getLockPtr():0) {}
	~PromiseResolution();

	using Promise<T>::Resolution::resolve;
	using Promise<T>::Resolution::reject;
	virtual void resolve(const T &result) throw() {
		Resolution *ptr = grabPtr();
		if (ptr) ptr->resolve(result);
	}
	virtual void resolve(const IConstructor<T> &result) throw() {
		Resolution *ptr = grabPtr();
		if (ptr) ptr->resolve(result);
	}
	virtual void reject(const PException &e) throw() {
		Resolution *ptr = grabPtr();
		if (ptr) ptr->reject(e);
	}

	const T &operator=(const T & v) {
		resolve(v);
		return v;
	}

protected:
	friend class Promise<T>;
	PromiseResolution(Future *r):ptr(r) {}

	Future *ptr;
	Future *grabPtr() {
		Future *p = ptr;
		forEachInstance(&setPtrToNull);
		return p;
	}

	static bool setPtrToNull(SharedResource *x, SharedResource *) {
		PromiseResolution<T> *c = static_cast<PromiseResolution<T> *>(x);
		c->ptr = 0;
		return true;
	}
};

template<>
class PromiseResolution<void>: public PromiseResolution<Empty> {
public:

	using PromiseResolution<Empty>::resolve;
	using PromiseResolution<Empty>::reject;
	void resolve() throw() {
		resolve(Empty());
	}
};


///Specialization for void argument.
/** Because void argument is a special type, we have specialization for it. Promises with void
 * type should accept callback functions without arguments and they should not expect return value of the
 * callback function. Also resolve promise should be called without arguments.
 *
 * Specialization uses Promise<Empty> where Empty is special empty class. During lifetime of
 * the promise, the Empty instance is carried through, but it should not leave the promise object out.
 *
 * Functions then() and thenCall() accepts callbacks without argument. Conversion to callback with
 * "the Empty" argument is done by the internal function EmptyCallVoid. This function shallows
 * the callback function and calls it without argument. It also doesn't expect return value.
 *
 * To resolve Promise<void> you should construct PromiseResolution<void>. This object
 * contains simple function resolve() without argument. Function internally constructs Empty instance
 * and uses it to process whole callback chain.
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
	Promise then(const PromiseResolution<void> &resolution);

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
	 * @exception TimeoutException timeout ellapsed before resolution
	 * @exception any promise has been rejected with an exception.
	 */
	void wait(const Timeout &tm) {Promise<Empty>::wait(tm);}

	Promise() {}
	Promise(const Promise<Empty> &x):Promise<Empty>(x) {}
	Promise(PromiseResolution<void> &resolution):Promise<Empty>(resolution) {}
	Promise(PromiseResolution<void> &resolution, IRuntimeAlloc &alloc):Promise<Empty>(resolution,alloc) {}

};


}

