/*
 * ijobcontrol.h
 *
 *  Created on: 20.9.2014
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_ACTIONS_IJOBCONTROL_H_
#define LIGHTSPEED_BASE_ACTIONS_IJOBCONTROL_H_
#include "message.h"

namespace LightSpeed {

///Controls single job, which can be also executor or thread

class IJobControl {
public:

	typedef Action JobFunction;
	typedef Action::Ifc IJobFunction;


	///Executes job
	/**
	 *
	 * @param fn function to execute.
	 *
	 * Job must be in "not running" state. Executing already running job can cause exception. Note that especially executors
	 * can report "running state" (through isRunning) and execute can be still available that because executors can
	 * handle multiple jobs at time.
	 */
	virtual void execute(const IJobFunction &fn) = 0;


	///Orders job to finish.
	/**
	 * Once finish is ordered, job should finish as soon as possible. Function also should interrupt any current waiting,
	 * but it should not interrupt any future waiting. Job should not to enter into wait state during finish, but waiting
	 * should be still need possible.
	 *
	 * Send finish to executor handling multiple jobs causes finishing all running jobs. There is no definition how
	 * executor handles new jobs after finish is issues.
	 */
	virtual void finish() = 0;

	///Synchronizes current thread with the job waiting to job's finish
	/** Blocks execution, until job finishes. If there is multiple jobs, waits until all jobs finish.
	 *
	 *
	 * */
	virtual void join() = 0;


	///Returns true, if job is still running
	/**
	 * @retval true job running. This state can change to false anytime after function returns.
	 * @retval false job is not running, this state should not change to true until job is manually restarted
	 *
	 * If there are multiple jobs, function returns true.
	 */
	virtual bool isRunning() const = 0;


	virtual ~IJobControl() {}

};


}



#endif /* LIGHTSPEED_BASE_ACTIONS_IJOBCONTROL_H_ */
