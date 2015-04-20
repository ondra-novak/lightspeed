#pragma once

namespace LightSpeed {


///Abstract thread hook which is executed on various thread events
/**
 * Thread hooks helps in case, when thread local variables are used and when they
 * require a special care on various thread events, such a thread creation,
 * destroying or exiting through an exception.
 *
 * Thread hooks are application global once they are installed. Every thread
 * created or destroyed during period of the hook installation causes that
 * special handler of the hook is called to perform anything that is required
 * during that event.
 *
 */
class Thread;

class AbstractThreadHook {
public:


	///Initiallizes thread hook.
	/**
	 * Thread hook is always initialized without installing it into current thread
	 */
	AbstractThreadHook();

	~AbstractThreadHook();

	///function returns true, whether the thread hook is installed
	/**
	 * @retval true installed
	 * @retval false not installed
	 */
	bool isInstalled() const;

	///Installs hook
	/**
	 * @note You cannot install already installed hook.
	 * Installing already installed hook is silently ignored
	 *
	 * @note hooks are executed in order of their installation.
	 */
	void install();

	///Uninstalls hook
	void uninstall();

	///Called when thread is initialized
	/**
	 * @param thread thread identifier.
	 *
	 * @exception any Handler can throw exception which causes that execution
	 * is stopped and onThreadException is called on already initialized hooks
	 *
	 * @note onThreadInit is called in order of the installation of the hooks
	 *
	 * @note Function is called inside a lock so it don't need to by MT Safe
	 */
	virtual void onThreadInit(Thread &) {}

	///Called when thread is about exit
	/**
	 * @param thread thread identifier.
	 *
	 * @exception any Handler can throw an exception which cause that
	 * other still initialized hooks will receive onThreadException
	 *
	 * @note onThreadExit is called in reversed order of the installation of the hooks
	 *
	 * @note Function is called inside a lock so it don't need to by MT Safe
	 */
	virtual void onThreadExit(Thread &)  {}

	///Called when thread terminates due unexpected exception after onThreadException is processed
	/**
	 * Function is equivaled to onThreadExit, just with difference that it is called on exception.
	 * Also note that you cannot throw exceptions here.
	 *
	 * @param thread
	 *
	 * @note Function is called inside a lock so it don't need to by MT Safe
	 *
	 * @note function is called inside of exception handler. Use this knowledge to determine which exception has been thrown.
	 *
	 * @note function is not called on master thread and on attached thread when ordinary exception is thrown, because these
	 * exception are not caught by the thread's boostrap function. Instead of onThreadExit is called as result of destruction
	 * of the Thread object during the stack unwind. However the onThreadException is called when exception is thrown from
	 * the onThreadInit or from the onThreadExit.
	 *
	 */
	virtual void onThreadException(Thread &) throw() {}


	static void callOnThreadInitHooks(Thread &thread);
	static void callOnThreadExitHooks(Thread &thread);
	static void callOnThreadExceptionHooks(Thread &thread) throw() ;

	AbstractThreadHook *getNextHook() const {return next;}
protected:

	bool installed;
	AbstractThreadHook *next;
};


}
