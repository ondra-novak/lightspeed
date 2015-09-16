/*
 * dispatcher.h
 *
 *  Created on: 16. 9. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_EXCEPTIONS_DISPATCHER_H_
#define LIGHTSPEED_MT_EXCEPTIONS_DISPATCHER_H_

#include "../../base/exceptions/systemException.h"
#include "../threadId.h"


namespace LightSpeed {

class NoCurrentDispatcherException: public ThreadException {
public:

    NoCurrentDispatcherException(const ProgramLocation &loc, ThreadId threadId)
        :Exception(loc), ThreadException(loc),
        threadId(threadId) {}

    ThreadId getThreadId() const {return threadId;}

	static LIGHTSPEED_EXPORT const char *msgText;;
protected:

    ThreadId threadId;

    void message(ExceptionMsg &msg) const {
    	msg(msgText);
    }

    LIGHTSPEED_EXCEPTIONFINAL;
    virtual const char *getStaticDesc() const {
        return "Currently running thread has no associated dispatcher";
    }
};


}




#endif /* LIGHTSPEED_MT_EXCEPTIONS_DISPATCHER_H_ */
