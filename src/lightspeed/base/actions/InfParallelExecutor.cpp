/*
 * InfParallelExecutor.cpp
 *
 *  Created on: Nov 6, 2012
 *      Author: ondra
 */


#include "InfParallelExecutor.h"
#include "../containers/autoArray.tcc"


namespace LightSpeed {

void InfParallelExecutor::execute(const IExecAction& action) {
	for (natural cnt = threadPool.length(), i = 0; i < cnt; i++) {
		const PThread &th = threadPool[i];
		if (!th->isRunning()) {
			th->join();
			th->start(action);
			return;
		}
	}

	PThread p = new DynThread;
	p->start(action);
	threadPool.add(p);

}

bool InfParallelExecutor::stopAll(natural timeout) {
	for (natural cnt = threadPool.length(), i = 0; i < cnt; i++) {
		threadPool[i]->finish();
	}
	for (natural cnt = threadPool.length(), i = 0; i < cnt; i++) {
		if (!threadPool[i]->getJoinObject().wait(timeout))
			return false;
		threadPool[i]->join();
	}
	return true;
}

void InfParallelExecutor::join() {
	stopAll(naturalNull);
}


void InfParallelExecutor::finish() {
	for (natural cnt = threadPool.length(), i = 0; i < cnt; i++) {
		const PThread &th = threadPool[i];
		if (th->isRunning()) {
			th->finish();
		}
	}

}

bool InfParallelExecutor::isRunning() const {
	return !threadPool.empty();
}


}
