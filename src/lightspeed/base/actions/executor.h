/*
 * executor.h
 *
 *  Created on: 4.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ACTIONS_EXECUTOR_H_
#define LIGHTSPEED_ACTIONS_EXECUTOR_H_

#include "message.h"
#include "ijobcontrol.h"

namespace LightSpeed {




	class IExecutor: public IJobControl {
	public:

		typedef JobFunction ExecAction;
		typedef IJobFunction IExecAction;

		///Executes action.
		/** Depend on type of executor, function can start action
		 * synchronous or asynchronous. Task can be scheduled into
		 * the queue and processed later 
		 * @param action action to execute
		 *
		 * @note Function is often not thread safe. Executor expects, that
		 * is controlled by single thread only. If you want to access
		 * executor from multiple threads, use proper synchronization
		 */
		virtual void execute(const IExecAction &action) = 0;
		///Stops processing all pending actions
		/** 
		 * Function stops all pending actions and does return
		 * when executor is not executing any action. This method
		 * is not generally MT safe, so don't call execute and stop
		 * at the same time. 
		 *
		 * After command stops all tasks, new tasks can be executed 
		 * with no limitation. 
		 *
		 * @note Function is often not thread safe. Executor expects, that
		 * is controlled by single thread only. If you want to access
		 * executor from multiple threads, use proper synchronization
		 */
		virtual bool stopAll(natural timeout = 0) = 0;


		virtual ~IExecutor() {}


	};

}

#endif /* LIGHTSPEED_ACTIONS_EXECUTOR_H_ */
