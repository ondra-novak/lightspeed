/*
 * ParallelTest.cpp
 *
 *  Created on: 14.5.2013
 *      Author: ondra
 */

#include "ParallelTest.h"
#include "../lightspeed/base/debug/dbglog.h"
#include "../lightspeed/base/actions/parallelExecutor.h"
#include "../lightspeed/mt/thread.h"

using LightSpeed::LogObject;

namespace LightSpeedTest {

class MyParEx: public ParallelExecutor {
public:
	MyParEx(natural maxThreads = 0,
			 	 	  natural maxWaitTimeout = naturalNull,
			 	 	  natural newThreadTimeout = 0,
			 	 	  natural threadIdleTimeout = naturalNull)
		:ParallelExecutor(maxThreads,maxWaitTimeout,newThreadTimeout,threadIdleTimeout) {}
	virtual void threadInit() {
		LogObject lg(THISLOCATION);
		lg.info("New thread");

	}

	virtual void threadDone() {
		LogObject lg(THISLOCATION);
		Thread::sleep(5000);
		lg.info("Thread exit");
	}

};

integer ParallelTest::start(const Args& ) {

	LogObject lg(THISLOCATION);
	lg.info("Test start"); {
		MyParEx par(20,naturalNull,0,500);
		for (natural i = 2000; i > 0; i = i -1) {
			par.execute(ParallelExecutor::ExecAction::create(this,&ParallelTest::worker,i/*rand() % 10000*/));
			if ( i % 300 == 0)
				Thread::deepSleep(3000);
		}
		par.stopAll(naturalNull);
	}
	lg.info("Test finished");


	return 0;

}

void ParallelTest::worker(natural id) {
	LogObject lg(THISLOCATION);
	lg.info("Starting worker: %1") << id;
	Thread::sleep(id/10+1);
	lg.info("Worker finish: %1") << id;

}


} /* namespace LightSpeedTest */


