/*
 * paralelExecutor.h
 *
 *  Created on: 4.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ACTIONS_PARALELEXECUTOR_H_
#define LIGHTSPEED_ACTIONS_PARALELEXECUTOR_H_

#include "message.h"
#include "executor.h"
#include "../../mt/thread.h"
#include "../memory/rtAlloc.h"
#include "../containers/autoArray.h"
#include "parallelExecutor2.h"



namespace LightSpeed {


	///Parallel executor is tool for add basic  parallelism into the application
	/**
	 *  Object implements IExecutor interface.
	 *
	 *  To use ParallelExecutor, construct instance of this class at stack
	 *  where you need the start parallel operation. You can specify count
	 *  of threads that will be used, and timeout for waiting for particular task
	 *
	 *  @code
	 *  ParallelExecutor parallel;
	 *  for (int i = 0; i < 1000; i++) {
	 *  	parallel.execute(Action(&foo,FooParams));
	 *  }
	 *  @endcode
	 *
	 *  Example calls function foo(const FooParams &) repeatedly for 1000 times.
	 *  ParallelExecutor allows to execute this function at every available
	 *  CPU on computer
	 *
	 *  Class itself tracks idle worker-threads and assign them a work. If
	 *  there is no idle worker-thread, function  waits until any
	 *  worker finishes and becomes idle.
	 *
	 *  Class doesn't support thread automatic shutdown on idle. Instance is not
	 *  also MT safe - this mean, that there should be only one thread that
	 *  control the object. Function executed inside worker-thread should not
	 *  access the object without proper synchronization. These limitations
	 *  allows keep implementation the simplest as possible (no locks and mutexes
	 *  on interface)
	 *
	 *
	 *
	 *
	 */
	class ParallelExecutor: public ParallelExecutor2 {
	public:


		///Constructs parallel executor
		/**
		 * @param maxThreads maximal count of threads. Default value 0 means
		 * 	that count of thread is equal to count of available CPUs. To
		 *  get count of CPUs use getCPUCount() function
		 *
		 * @param maxWaitTimeout  how long execute() can wait for
		 *  idle worker in miliseconds. Use naturalNull to infinite waiting
		 */
		ParallelExecutor(natural maxThreads = 0,
						 natural maxWaitTimeout = naturalNull);


		///detects count of cpus
		/**
		 * @return count of processors available for this application.
		 * Function can return 0, if count cannot be determined.
		 *
		 * Constructor of ParallelExecutor() uses this function while maxThreads is
		 * set to zero. In case that function returns 0, ParallelExecutor initializes
		 * itself to 1.
		 *
		 */
		static natural getCPUCount();

	};


}


#endif /* LIGHTSPEED_ACTIONS_PARALELEXECUTOR_H_ */
