/*
 * directExecutor.h
 *
 *  Created on: 4.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ACTIONS_DIRECTEXECUTOR_H_
#define LIGHTSPEED_ACTIONS_DIRECTEXECUTOR_H_


#include "executor.h"

namespace LightSpeed {

	class DirectExecutor: public IExecutor {
	public:
		virtual void execute(const IExecAction &action) {
			runn = true;
			try {
				action();
				runn = false;
			} catch (...) {
				runn = false;
				throw;
			}
		}
		virtual bool stopAll(natural ) {
			return true;
		}

		virtual void join() {}

		virtual void finish() {};

		virtual bool isRunning() const {
			return runn;
		}

		DirectExecutor():runn(false) {}
	protected:
		bool runn;


	};


}

#endif /* LIGHTSPEED_ACTIONS_DIRECTEXECUTOR_H_ */
