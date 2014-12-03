/*
 * messages.cpp
 *
 *  Created on: 2.1.2011
 *      Author: ondra
 */


//#include "uncaughtException.h"
#include "fiberException.h"
#include "timeoutException.h"
#include "threadException.h"
#include "../process.h"

namespace LightSpeed {


//const char *ThreadUncaughtException::msgText =  "Exception thrown from the thread: %1";
 const char *NoCurrentFiberException::msgText = "No current fiber - there should be at least one MasterFiber";
 const char *FiberNotRunningException::msgText = "Fiber #%1 not running now (or already finished)";
 const char *FiberErrorException::msgText = "Fiber error: ";
 const char *TimeoutException::msgText = "Timeout exception";
 const char *ThreadBusyException::msgText = "Thread object is busy (exception)";
 const char *NoCurrentThreadException::msgText = "No Thread instance is associated with the current thread";
 const char *ThreadNotStartedException::msgText = "Operation is not defined because thread did not started yet";
 const char *ThreadNotRunningException::msgText = "Operation is not defined because thread is not running";
 const char *UnableToStartProcessException::msgText = "Unable to start process '%1' ";
}
