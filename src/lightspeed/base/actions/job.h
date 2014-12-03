/*
 * job.h
 *
 *  Created on: 19. 9. 2014
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ACTION_JOB_H_
#define LIGHTSPEED_BASE_ACTION_JOB_H_
#include "../../mt/sleepingobject.h"
#include "../memory/allocPointer.h"
#include "../exceptions/genexcept.h"
#include "ijobcontrol.h"

namespace LightSpeed {



///Job object - allows to control single job
/**
 * Job is sequence of action started when event happen
 * Job can be executed and then suspended and resumed on event. Execution is defined by function
 *
 * In compare to Thread of Fiber, Jobs needs a less resource, but it can be harder to
 * implement, because anytime job is suspended, content of its stack is discarded. So
 * job needs additional object that defines its internal state. Job object itself
 * can be part of this internal state.
 *
 * Job can be resumed immediatelly on event, or through ISleepingObject or IExecutor
 * This allow to handle better scheduling if event happen on different thread than expected
 *
 *
 */
class Job: public ISleepingObject, public IJobControl {
public:



	///Constructs simple job, which is executed immeditally with the event
	/** Job is started immetiatelly when wakeUp is called. This is
	 * great for simple single-threaded application. Multi-threaded application
	 * just may need to execute job on specific thread. In this case
	 * it is better to bind job to the thread or executor
	 */
	Job();
	///Constructs job which execution is controled through another thread or sleeping object
	/**
	 * @param jobWakeup pointer to sleeping object responsible to wakeup job. Object shuld
	 * call run function as soon as posible, but asynchronous.
	 */
	Job(ISleepingObject *jobWakeup);
	///Constructs job which executyion is controled through an executor
	/**
	 * @param jobWakeup pointer to executor. When event arrives, job is resumed through that executor.
	 */
	Job(IJobControl *jobWakeup);



	///You can assign one job to anothed
	/**
	 * Note that previous job is stopped if needed.
	 *
	 * @param source source job
	 * @return
	 */
	Job &operator=(const Job &source);

	///Binds job to an sleeping object
	/**
	 * @param jobWakeup pointer to sleeping object which executes job on event
	 */
	void bindTo(ISleepingObject *jobWakeup);
	///Binds job to an executor
	/**
	 *
	 * @param jobWakeup pointer to executir object which executes job on event
	 */
	void bindTo(IJobControl *jobWakeup);
	///Unbinds job object
	/**
	 * When job object is not bound, it is executed immediatelly in context of the event handler
	 */
	void unbind();


	///Executes job - IExecutor implementation
	/**
	 * @param action action to execute
	 * @exception UnexpectedJobState you are trying to start already running job
	 *
	 * Function executes job immediatelly returning from execution once job sleeps or terminates
	 */
	virtual void execute(const IJobFunction &action);
	///Postpones execution of the job
	/**
	 * Job must be in state "running". Function is not MT safe
	 *
	 * @param action reference to function which will be started on wakeUp
	 * @exception UnexpectedJobState you are trying to sleep job not in run state
	 *
	 * @note if job is not bound, function can executed immediatelly if there were event arrived during run phase
	 */
	void sleep(const IJobFunction &action);


	///Stops jobs
	/**
	 * @exception UnexpectedJobState you can only stop job if it is in following states: sleeping and ready. After stopping
	 * the job, job receives "stop" state
	 */
	void stop();

	///runs jobs if it is necesery
	bool run();


	///wakes up job
	/**
	 * Depend on configuration, job enters into either ready or running state. If job is bound,
	 * next state is "ready" and underlying manager is notified about event. Unbound job receives "running" state and
	 * it is immediatelly executed.
	 * @param reason
	 */
	virtual void wakeUp(natural reason = 0) throw();


	virtual void join();

	virtual void finish();

	enum State {
		///job is not running and can be started
		not_running,
		///job is running now
		running,
		///job is running and there is event recorded that causes that job will not sleep in next perior
		running_event,
		///job sleeping, can be waken up
		sleeping,
		///job has been waken and it is ready to run
		ready,
	};

	State getState() const;


	static const char *unexpectedJobStateMsg;
protected:

	atomic state;
	JobFunction jobFn;
	Pointer<ISleepingObject> ntf_so;
	Pointer<IJobControl> ntf_exec;
	natural reason;

	State setState(State oldState, State newState);
	State setState(State oldState1, State newState1,State oldState2, State newState2 );

};

typedef GenException1<Job::unexpectedJobStateMsg, Job::State> UnexpectedJobState;

} /* namespace LightSpeed */

#endif /* LIGHTSPEED_BASE_ACTION_JOB_H_ */
