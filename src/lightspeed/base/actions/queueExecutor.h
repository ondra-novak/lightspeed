/*
 * queueExecutor.h
 *
 *  Created on: 13. 7. 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ACTIONS_QUEUEEXECUTOR_H_
#define LIGHTSPEED_BASE_ACTIONS_QUEUEEXECUTOR_H_
#include "../containers/queue.h"
#include "executor.h"
#include "../../mt/gate.h"
#include "../../mt/semaphore.h"
#include "../memory/sharedPtr.h"

namespace LightSpeed {


///QueueExecutor implements queue on thread to convert it into simple dispatcher
/** Because it is executor, you can only executore actions. It is recomended to
 *  include Promise and try-catch handler into the executed routine, otherwise
 *  execeptions can be thrown out of the executor in unexpected thread.
 *
 */
class QueueExecutor: public IExecutor {
public:

	QueueExecutor();
	~QueueExecutor();


	///Defines executors thread
	/** Executor's thread carries out actions. You have to define at least one
	 * thread. Multiple threads are also possible.
	 *
	 * Calling this function is thread defined. Function is exited when executor is
	 * destroyed, or stopAll(), finish() or join() is called
	 */
	void serve();


	///Execute action
	/**
	 * Function just execute action. If you need to retrieve value from the action, you should
	 * include Promise as argument of the action. It is also advised to include try-catch handler
	 * otherwise exception is thrown inside executor's thread causing the thread to leave runThread() function.
	 *
	 * @param action action to execute. Action is queued. There is no way to remove action from the queue.
	 * If you need to have a way to cancel queued action, just simply execute Promise<void>::resolve(). You
	 * can attach handlers to the associated Future<void>, and the Future can be canceled. Handlers defined
	 * by the Future are executed in the content of the executor thread
	 *
	 * @note if executor is stopped or destroyed, all actions are also destroyed without execution. Detecting
	 * of this situation is on you. If you use Future-Promise, then Promise is reject if last
	 * instance of the Promise is destroyed without resolution.
	 */
	virtual void execute(const IExecAction &action);

	///Flushes queue and stop all threads
	virtual bool stopAll(natural timeout = 0) ;

	///Function is called when thread is idle.
	/**
 	 * @param counter count of calls when no message has been processed
	 * @return timeout to wait before next onIdle is called again. However, this can
	 * be sooner when message arrives.
	 */
	virtual Timeout onIdle(natural counter);

	///Sends all threads out of executor and flushes the queue
	virtual void finish();

	///Waits until all threads leaves executor
	virtual void join();

	///returns true, if there at least one message processing
	virtual bool isRunning() const;

	///Resets finish flag enabling to enqueue actions again
	void reset();
protected:

	///lock of internals
	FastLock lock;
	///gate is opened, if no threads are serving inside
	Gate noThreads;
	///queue of messages
	Queue<SharedPtr<IExecAction> > queue;
	///point where all threads waiting for a new action
	Semaphore semaphore;
	///count of running messages
	natural runningMessages;
	///count of serving threads
	natural threadsIn;
	///true if finish is signaled (someone may wait on the gate)
	bool finishFlag;

	virtual void wakeThread();

	class Server;



};



}



#endif /* LIGHTSPEED_BASE_ACTIONS_QUEUEEXECUTOR_H_ */
