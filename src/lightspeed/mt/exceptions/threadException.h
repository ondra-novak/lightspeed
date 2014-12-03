/*
 * threadException.h
 *
 *  Created on: 11.9.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_MT_THREADEXCEPTION_H_
#define _LIGHTSPEED_MT_THREADEXCEPTION_H_

#include "../../base/exceptions/systemException.h"
#include "../threadId.h"


namespace LightSpeed {

    class Thread;

    class ThreadException: virtual public Exception {
    public:
        ThreadException(const ProgramLocation &loc):Exception(loc) {}
        ThreadException():Exception(THISLOCATION) {}
    };


    class ThreadBusyException: public ThreadException {
    public:
        ThreadBusyException(const ProgramLocation &loc)
            :Exception(loc){}

        LIGHTSPEED_EXCEPTIONFINAL;
    	static LIGHTSPEED_EXPORT const char *msgText;;

    protected:

        void message(ExceptionMsg &msg) const {
        	msg(msgText);
        }
    };

    class UnableToStartThreadException: public ThreadException, public ErrNoWithDescException {

    public:
        UnableToStartThreadException(const ProgramLocation &loc, int err)
            :Exception(loc)
            ,ErrNoWithDescException(loc,err,String(L"Unable to start thread")) {}

        LIGHTSPEED_EXCEPTIONFINAL;
            virtual const char *getStaticDesc() const {
                return "System rejected the request to star new thread. "
                    "This situation is not expected and cannot be handled. "
                    "Explore the error code to see what has happened.";
            }

            void message(ExceptionMsg &msg) const {
                ErrNoWithDescException::message(msg);
            }
    };

    class NoCurrentThreadException: public ThreadException {
    public:

        NoCurrentThreadException(const ProgramLocation &loc, ThreadId threadId)
            :Exception(loc), ThreadException(loc),
            threadId(threadId) {}

        ThreadId getThreadId() const {return threadId;}

    	static LIGHTSPEED_EXPORT const char *msgText;;
    protected:

        ThreadId threadId;

        void message(ExceptionMsg &msg) const {
        	msg(msgText);
        }

/*TODO        virtual void getMsgText(std::ostream &msg) const {
            msg << "Thread #" << ((const char *)toAsciiC(threadId)) << " has no Thread instance associated. ";
        }*/

        LIGHTSPEED_EXCEPTIONFINAL;
        virtual const char *getStaticDesc() const {
            return "Currently running thread has not been created by the LightSpeed library. "
                "Every thread in the LightSpeed library need an instance of the Thread class "
                "associated with it. Current thread has none. To allow to run foreing threads "
                "inside of the LightSpeed library, the very first action of such thread should "
                "be function Thread::attach(). This will associate choosen instance with the "
                "thread and you will able to use LightSpeed objects to work with this thread. ";
        }
    };

    ///Some functions require running or finished thread - cannot be called before thread has not started yet.
    class ThreadNotStartedException: public ThreadException {
    public:
    	ThreadNotStartedException(const ProgramLocation &loc):ThreadException(loc) {}
    	LIGHTSPEED_EXCEPTIONFINAL;

    	static LIGHTSPEED_EXPORT const char *msgText;;

    	virtual void message(ExceptionMsg &msg) const {
    		msg(msgText);
    	}

    };

    class ThreadNotRunningException: public ThreadException {
    public:
    	ThreadNotRunningException(const ProgramLocation &loc):ThreadException(loc) {}
    	LIGHTSPEED_EXCEPTIONFINAL;

    	static LIGHTSPEED_EXPORT const char *msgText;;

    	virtual void message(ExceptionMsg &msg) const {
    		msg(msgText);
    	}

    };

}

#endif /* _LIGHTSPEED_THREADEXCEPTION_H_ */
