#pragma once

#include "../../base/exceptions/genexcept.h"

namespace LightSpeed {


extern const char *linuxThreads_signalExceptionMsg;


///Object that describes unexpected signal event under linux environment
/**
 * This is not really exception, because you are unable to catch it anywhere.
 *
 * When thread causes an unexpected signal, signal handler forwards execution
 * through longjmp to special branch of the thread-bootstrap section, where
 * signal is transformed to an instance of UnexpectedSignalException. Then
 * this object is passed as argument of the function onThreadException declared
 * on IApp interface of the current application. Application can handle exception
 * similar as any other unhandled exception. For example it can log this event.
 * Because there is high possibility that integrity of the application has
 * been corrupted, program is terminated immediatelly after onThreadException
 * returns through std::terminate();
 */
typedef GenException1<linuxThreads_signalExceptionMsg, int> UnexpectedSignalException;

}
