/*
 * fiber.h
 *
 *  Created on: 8.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_FIBER_H_
#define LIGHTSPEED_MT_FIBER_H_

#include "../base/types.h"
#include "../base/exceptions/systemException.h"
#include "../base/actions/message.h"
#include "../base/memory/sharedResource.h"
#include "sleepingobject.h"

namespace LightSpeed {

	class FiberContext;
	class MasterFiber;

    typedef  Action FiberFunction;
    typedef  Action::Ifc IFiberFunction;


    ///Allows multiple contexts in single thread.
	class Fiber: public ISleepingObject {
	public:

    	///Constructor
		Fiber();
		///Destructor
		/**
		 * It is allowed to destroy running fiber. Destructor calls stop()
		 * before fiber is destroyed.
		 *
		 * @note Every inherited class should explicitly call stop() in the
		 * destructor!
		 *
		 */
		virtual ~Fiber();

		///Starts the fiber
		/**
		 * Starts the fiber calling the specified function. Function
		 * returns when fiber exits or when fiber calls sleep() function
		 *
		 * @param fn function to call
		 * @param stackSize size of stack, if zero default stack is used
		 *
		 * @note When fiber exits, master fiber is waken up
		 */
		void start(const IFiberFunction &fn, natural stackSize = 0);
		///Stops the fiber
		/**
		 * Stops the fiber posting the special uncatchable exception.
		 * It should unwind stack and return to bootstrap. This exception
		 * is not treat as uncaught, because it is caught by
		 * bootstrap function
		 \*/
		void stop();

		///Resumes fiber
		/**
		 * Gives control to the other fiber. Function returns when the other
		 * fiber sleeps or wakes up current fiber.
		 *
		 * Trying to resume passive fiber causes immediate return.
		 *
		 * @note function fails if called during exception rollback. Failure
		 * cannot be reported, function exits immediately instead.
		 */
		void wakeUp(natural reason = 0) throw();

		///Checks fiber state
		/**
		 * Because wakeUp doesn't allow exceptions, there should be another
		 * mechanism how to check state of fiber. This function checks
		 * state of fiber and/or throws any uncaught exception made by fiber
		 *
		 * Typical cycle running the fibbrs
		 *
		 * @code
		 * while (f1.checkState() || f2.checkState() || f3.checkState()) {
		 *   f1.wakeUp();
		 *   f2.wakeUp();
		 *   f3.wakeUp();
		 * }
		 * @endcode
		 *
		 * there is overloaded operator bool doing the same check, so code
		 * can be shorten
		 *
		 * @code
		 * while (f1 || f2 || f3) {
		 *   f1.wakeUp();
		 *   f2.wakeUp();
		 *   f3.wakeUp();
		 * }
		 * @endcode
		 *
		 * @retval true active, wakeUp() will work
		 * @retval false not active, wakeUp immediate returns
		 * @exception any If called from master fiber, function throws
		 * uncaught exception detected in this fiber. Using this function
		 * from other fibers doesn't throw any exception. Fiber terminated
		 * with uncaught exception is shown as passive.
		 *
		 * @note Function doesn't generate exception if called during
		 *  exception stack unwind.
		 *
		 *  @note Exception is thrown once only. Repeatedly call of checkState()
		 *  on same fiber doesn't thrown exception.
		 */
		bool checkState() const;

		///Returns state of fiber
		/**
		 * @see checkState()
		 */
		operator bool() const {return checkState();}

		///Returns inverted state of fiber
		/**
		 * @see checkState()
		 */
		bool operator!() const {return !checkState();}

		///Returns true, if current fiber is master fiber
		/**
		 * @retval true current fiber is master fiber
		 * @retval false current fiber is not master fiber
		 */
		bool isMaster() const;

		///Retrieves current fiber instance
		/**
		 * @return current fiber instance
		 */
		static Fiber &current();

		static Fiber *currentPtr();
		///Sleeps current fiber returning control to the caller
		/**
		 * Everytime fiber is waken, reference to the caller is remembered.
		 * Calling function sleep() causes, that caller is waken up and
		 * current fiber starts sleeping
		 *
		 * There is only one reference to the caller, so you cannot make
		 * any recursion calling. So multiple wakes causes, that
		 * last caller overwrites reference to the previous caller. Keep
		 * hierarchy between your fibers, or organize fibers into
		 * master-slave relation, whether only master calls wakeUp() and
		 * slave calls sleep();
		 *
		 * @return function returns reason sent by wakeUp() function from
		 * the caller.
		 *
		 * @note function fails if called during stack unwind due
		 * uncaught exception because global variables used during unwind
		 * are not swapped with context.
		 */
		static natural sleep();

		///retrieves address of caller
		/**
		 *
		 * @return Address of fiber that caused last resume of this fiber.
		 *  Note, return value may be NULL for master fiber.
		 */
		Fiber *getCaller();

		///Retrieves address of master fiber
		Fiber *getMaster();

		///special exception - cannot be caught by std::exception - to force fiber stop
		enum StopFiberException {stopFiberException};

		///creates master fiber
		/** Master fiber is instance of this object created internally to
		 * have instance that is associated with current running context.
		 *
		 * This function creates this instance for current thread. By default,
		 * master fiber is created on first call Fiber::start(). Sometimes
		 * can be useful to create master fiber explicitly, for example,
		 * if you want to resume to the fiber created in different thread.
		 * In this case you will receive exception when master fiber is not ready.
		 *
		 * Master fiber is destroyed with the thread. You can destroy master
		 * thread explicitly using function destroyMasterFiber()
		 *
		 * @return pointer to master fiber. If master fiber is already created,
		 * function returns instance of the fiber similar to function getMaster()
		 */
		static Fiber &createMasterFiber();

		///Destroys master fiber.
		/** releases memory for master fiber. Function must be called in context
		 * which has been used to creating the master fiber. Otherwise it fails
		 *
		 * @retval true success, master fiber has been destroyed, or did not existed
		 * @retval false failure, master fiber cannot be destroyed now, because
		 *   is not current
		 *
		 * @note master fiber is released before thread exits.
		 */
		static bool destroyMasterFiber();

	protected:

		///context of fiber - internal structure
		FiberContext *ctx;
		///caller of fiber - to yield()
		Fiber *caller;
		///master fiber - master fiber has this here
		Fiber *master;
		///stored exception
		mutable PException exitException;

		friend class FiberContext;

		static void switchFibers(Fiber *from, Fiber *to);

	private:
		Fiber(const Fiber &other);
		Fiber &operator=(const Fiber &other);

	};


}  // namespace LightSpeed

#endif /* LIGHTSPEED_MT_FIBER_H_ */
