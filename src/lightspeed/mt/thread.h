/*
 * fthread.h
 *
 *  Created on: 22.8.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_FTHREAD_H_
#define LIGHTSPEED_MT_FTHREAD_H_
#include "../base/actions/message.h"
#include "sleepingobject.h"
#include "timeout.h"
#include "threadId.h"
#include "../base/sync/tls.h"
#include "gate.h"
#include "../base/exceptions/exception.h"
#include "../base/containers/arrayref.h"
#include "../base/containers/optional.h"
#include "threadSleeper.h"

namespace LightSpeed {


	///context of the thread
	class ThreadContext;

	typedef Action ThreadFunction;
	typedef Action::Ifc IThreadFunction;


	///New thread class
	/**
	 * Instead of old implementation, this class implements threads
	 * more closely then old one. Thread state is complette written into
	 * this object or into stack in the running thread. No memory allocations
	 * are done. Thread cannot be shared or copied, you have to handle by
	 * own.
	 *
	 * Thread running when it is constructed and cannot be restarted.
	 * This simplyfies usage in simple cases
	 */
	class Thread: public ISleepingObject {

	public:



		///Creates thread and calls function
		/**
		 * @param fn function to call inside the thread
		 */
		Thread(const IThreadFunction &fn);

		///Creates thread and calls function - you can specify stack size
		/**
		 * @param fn function to call inside the thread
		 * @param stackSize size of stack in "naturals"
		 */
		Thread(const IThreadFunction &fn, natural stackSize);

		///Creates suspended thread
		/**
		 * @note some function may report this thread finished. Only allowed
		 * operations for this type of threads are attach() or start()
		 *
		 * @see attach(), start()
		 */
		Thread();
		///Destructor
		/**
		 * Destroys the thread. Only finished thread can be destroyed. Function
		 * calls stop() to ensure, that thread is finished
		 */
		virtual ~Thread();


		///starts the new thread
		/**
		 * Function can be called only for newly created object. It is not
		 * recomended to call this function on object containing finished thread.
		 * @param fn function to start
		 */
		void start(const IThreadFunction &fn);

		///Creates thread and calls function - you can specify stack size
		/**
		 * @param fn function to call inside the thread
		 * @param stackSize size of stack in "naturals"
		 */
		void start(const IThreadFunction &fn, natural stackSize);


		///Attaches object to the current thread
		/**
		 * Function examines current thread and if already has thread object
		 * attached, it fails returning false
		 *
		 * Otherwise, it attaches current object to the thread.
		 *
		 * @param keepContext if true, context of thread is kept after thread object is destroyed. You can
		 * later reattach the context. All variables set to the TLS is kept with the context.
		 * Note on some platforms (Windows), context is not destroyed on thread exit, because there is no
		 * way how to detect thread termination and perform a cleanUp. In this case, contexts of dead threads are
		 * cleaned when next successful attach is called. If this argument is false, context is destroyed in destructor
		 *
		 * @param contextAlloc pointer to allocator used to allocate context. If zero, context is allocated 
		 * using standard allocator. Note that allocator must be valid during context is allocated, which can be non-deterministic
		 * time when keepContext is true. Also note that when keepContext is true, allocator must be thread safe
		 *
		 * @retval true successfully attached to this object
		 * @retval false failure, thread is already attached
		 */
		 
		bool attach(bool keepContext, IRuntimeAlloc *contextAlloc = 0);

		///Retrieves master thread
		/**
		 * Master thread is thread object created for main thread before other threads
		 * are created. If there is no such a object, function creates it now.
		 *
		 * @return reference to master thread
		 *
		 * @note always ensure, that first call of getMaster is inside of main thread.
		 * This is always fulfilled when threads are created using Thread class. If
		 * there is posibility of creating thread by different way, call getMaster()
		 * before that situation happen
		 *
		 */
		static Thread &getMaster();


		///Determines, whether application using threads
		/**
		 * @retval true application using threads, you can use Thread class
		 * in full range
		 * @return false application doesn't using threads (yet), so you can
		 * reach exception while using Thread class. You should be carefull
		 * while calling getMaster()
		 */
		static bool isThreaded();


		///Stops the thread
		/**
		 * it is combination of finish() and join()
		 *
		 * @note you cannot terminate the thread. Thread must always finish
		 * its work reaching end of user thread function, or throwing an exception.
		 * This function sets finish flag to true and then waits until thread
		 * finish. Function expects, that thread repeatedly checks state of
		 * finish flag.
		 *
		 * @see finish, canFinish, isFinishing
		 *
		 */
		void stop() {finish();join();}

		///Finalizes state of thread
		/**
		 * Function must be called to finalize state of not running thread.
		 * If called on running thread, function stops for infinite time until
		 * thread finish.
		 *
		 * If called on not running thread, determines its state. Possible exception
		 * is thrown at this point.
		 *
		 * In all cases, function cleans state of thread and releases all resources.
		 * Calling join() repeatedly on finished thread has no effect, but is not
		 * error.
		 *
		 * function is always called inside destructor of Thread(). If you ommit
		 * to call function manually, you can receive exception during destruction
		 * of the thread object.
		 *
		 */
		void join();

		///asks whether thread is running
		/**
		 * Function determines thread state. Thread is running only when thread
		 * executes user code including sleeping and waiting for synchronisation.
		 *
		 * Thread not started, finished or terminated due exception is reported as
		 * not running. To determine why thread is not running, call join().
		 *
		 *
		 * @retval true running
		 * @retval false finished, or exceptional
		 */
		bool isRunning() const;


		///Retrieves current thread
		/**
		 * @return reference to the current thread
		 * @exception NoCurrentThreadException throws, if function is called in thread, which
		 * is not controlled by Thread object, or in single-threaded application. To prevent
		 * this exception while using multi-threaded library inside single-threaded application,
		 * call getMaster() to install thread context on current thread
		 *
		 * @see getMaster, isThreaded, currentPtr
		 *
		 * @note function can be slow. Always cache result in local variable.
		 */
		static Thread &current();

		///Retrieves pointer to current thread
		/**
		 *
		 * @return pointer to thread, or NULL, if thread object is not associated
		 * @note function can be slow. Always cache result in local variable
		 * @see getMaster, isThreaded, current
		 */
		static Thread *currentPtr();

		///Sleeps current thread for timeout, or until wakeUp
		/**
		 * @param timeout timeout to sleep
		 * @retval true success,
		 * @retval false interrupted
		 * @see deepSleep
		 */
	    static bool sleep(const Timeout &timeout) {
	    	natural dummy;
	    	return current().impSleep(timeout,dummy);
	    }

		///Sleeps current thread for timeout, or until wakeUp
		/**
		 * @param timeout timeout to sleep
		 * @param reason reference to variable which receives reason of the interruption
		 * @retval true success,
		 * @retval false interrupted
		 * @see deepSleep
		 */
	    static bool sleep(const Timeout &timeout, natural &reason) {
	    	return current().impSleep(timeout,reason);
	    }



	    ///Sleeps current thread for timeout, cannot be interrupted
	    /**
	     * Function sleeps thread for specified timeout, which cannot be interrupted
	     * by function wakeUp. Function doesn't reset interruption flag. Interruption
	     * arrived during deepSleep can shortend next sleep()
	     *
	     * @param timeout defined time interval to sleep
	     * @see sleep
	     */
	    static void deepSleep(const Timeout &timeout);

				
		///Wakes up the thread
	    /**
	     * @param reason reason to sent with the wakeup
	     */
	    void wakeUp(natural reason = 0) throw();

	    ///Retrieves ID of thread
		ThreadId getThreadId() const;

		///signals thread to finish its work.
		/**
		 * @note thread must check state of finish-flag using function canFinish()
		 *
		 * @note Function also interrupts sleeping through wakeUp, while reason
		 * is set to naturalNull.
		 *
		 * @see canFinish,isFinishing
		 */
		void finish();

		///true, if CURRENT thread should finish
		/**
		 * Checks finish-flag of current thread.
		 * @retval true thread should finish its work and exits as soon as possible
		 * @retval false thread can go on
		 * @see canFinish,isFinishing
		 */
		static bool canFinish();

		///determines finish-flag of the thread
		/**
		 * @retval true finish-flag is set
		 * @retval false finish-flag is not set
		 * @see canFinish,isFinishing
		 */
		bool isFinishing() const;

		///retrieves reference to TLS table
		ITLSTable &getTls();

		///retrieves reference to TLS table
		const ITLSTable &getTls() const;

		///Retrieves reference to Gate object
		/** Gate object becomes signaled when thread finishes */
		Gate &getJoinObject();

		///Returns safe reference to ISleepingObject
		/**Because thread as instance of ISleepingObject may disappear once they are destroyed,
		 * other threads can cause crash accessing to released memory. From version 15.9, objects
		 * responsible to wakeUp thread are allocated as count-ref resources. You can now keep
		 * this object beyoind destroying original Thread without crash due accessing it (i.e. calling wakeUp)
		 *
		 * Calling wakeUp() while thread is destroyed is silently ignored. This helps to prevent
		 * many race conditions. However, there is still possiblity of a deadlock, when your
		 * thread depends on already dead thread, because sleeping object cannot wake him up.
		 *
		 * @return Reference to ThreadSleeper
		 */
		RefCntPtr<IThreadSleeper> getSafeSleepingObject();


	protected:

		friend class ThreadContext;
		typedef RefCntPtr<ThreadSleeper> Sleeper;

		static const natural flagFinish = 1;

		///pointer to thread context 
		/** points into structure which is platform depend
		@note pointer can point into stack of the thread and can
		 become invalid on thread exit
		*/
		ThreadContext * volatile threadContext;
		///gate become signaled when thread exits. Useful for join
		Gate joinObject;
		///contains various flags used by thread engine
		/** flags are not defined. Only exception is flagFinish,
		   which signals that other thread wants to finish this thread ,
		   note that there can be internal flags need by thread engine*/
		natural flags;

		///Thread's sleeper
		/** object initializes self during thread creation
		 * Do not send notifications until thread is created
		 */
		Sleeper sleeper;

		///thread's id
		ThreadId id;

		Thread(const Thread &other);
		Thread &operator=(const Thread &other);


		///Override function to implement own sleep. You can for example do idle tasks while sleeping
		virtual bool impSleep(const Timeout &tm, natural &reason);



	};


 	class AttachedThread:public Thread {
	public:
		AttachedThread() {
			attach(true);
		}
		AttachedThread(IRuntimeAlloc &alloc) {
			attach(true,&alloc);
		}

		AttachedThread(bool keepContext, IRuntimeAlloc &alloc) {
			attach(keepContext,&alloc);
		}
	};

}

#endif /* LIGHTSPEED_MT_FTHREAD_H_ */
