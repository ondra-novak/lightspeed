#pragma once
#include "../containers/optional.h"
#include "../containers/autoArray.h"
#include "../memory/smallAlloc.h"
#include "../exceptions/exception.h"
#include "../exceptions/canceledException.h"
#include "../constructor.h"
#include "../../mt/timeout.h"
#include "../../mt/sleepingobject.h"
#include "../../mt/exceptions/timeoutException.h"
#include "../../mt/fastlock.h"

namespace LightSpeed {


template<typename T> class Promise;
template<typename T> class PromiseResolution;



IRuntimeAlloc *getPromiseAlocator();



template<typename T>
class Promise {
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
	const T &wait(const Timeout &tm = Timeout()) const;

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

	///Registers additional callback 
	/** This allows to handle resolution by specific way */
	Promise registerCb(Resolution *ifc);
	///Unregisters callback
	/** This allows to handle resolution by specific way */
	Promise unregisterCb(Resolution *ifc);




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

	class Future:public RefCntObj, public Resolution, public DynObject  {
	public:
		
		Future(IRuntimeAlloc &alloc):alloc(alloc) {}
		bool checkResolved();
		virtual void resolve(const T &result) throw() ;
		virtual void resolve(const IConstructor<T> &result) throw ();
		virtual void reject(const PException &e) throw();
		void registerExpectant(Resolution *ifc);
		void unregisterExpectant(Resolution *ifc);
		IRuntimeAlloc &alloc;

		FastLock *getLockPtr();
	protected:
		FastLock lock;
		Optional<T> value;
		PException exception;
		AutoArray<Resolution *, SmallAlloc<4> > sleepers;


		template<typename X>
		void resolveInternal( const X & result );
	};

	RefCntPtr<Future> future;

};


template<class T>
class PromiseResolution: public SharedResource, public Promise<T>::Resolution {
public:

	typedef typename Promise<T>::Resolution Resolution;
	typedef typename Promise<T>::Future Future;

	PromiseResolution():ptr(0) {}
	PromiseResolution(const PromiseResolution &other):SharedResource(other,other.ptr?other.ptr->getLockPtr():0) {}
	~PromiseResolution() {
		if (!SharedResource::isShared() && ptr) {
			reject(CanceledException(THISLOCATION));
		} else {
			unshare(ptr?ptr->getLockPtr():0);
		}
	}

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

}


