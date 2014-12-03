#if 0
#ifndef LIGHTSPEED_LINUX_EVENTIMPL_H_
#define LIGHTSPEED_LINUX_EVENTIMPL_H_

#include <pthread.h>
#include "../../base/types.h"
#include "../../base/sync/synchronize.h"
#include "llmutex.h"


namespace LightSpeed {

//TODO consider sem_post and sem_wait usage


    class LLEvent: public LLMutex {
    public:

        LLEvent(bool manualReset = false):manualReset(manualReset) {
            int err = pthread_cond_init(&cond,0);
            if (err)
                throw ErrNoWithDescException(THISLOCATION,err,
                		String(L"LLEvent initialization failed"));
            state = false;
        
        }


        ///Destructor
        /**
         * Destroys the event object. Event object SHOULD not be destroyed
         * when there is waiting thread. To prevent this undefined situation,
         * destructor will TRY to evict the waiting thread before it is destroyed.
         * It sometime allowed to use this feature, if the caller known, that
         * there will be no more threads trying to get the event during destruction.
         *
         * @note All threads that waiting this object before it is destructed will
         * be notified and wait function will return true. Consider, if this
         * useful behavior for your task.
         */
        ~LLEvent() {
            //enable manual reset to allow eviction of all waiting threads
            manualReset = true;
            //notify object to evict all waiting threads
            notify();
            //now state is open - no more threads will try to catch event after destruction

            //lock the object taking the mutex before final destroy to ensure, that there is no thread inside
            Synchronized<LLMutex> _(*this);
            //destroy the condition variable
            pthread_cond_destroy(&cond);
            //unlock mutext and finalize destruction
        }

        bool wait(Timeout timeout = Timeout())  {
            Synchronized<LLMutex> _(*this);
            
            while (!state) {
                if (timeout.isInfinite()) {
                    int err = pthread_cond_wait(&cond,getMutex());
                    if (err)
                        throw ErrNoWithDescException(THISLOCATION,err,
                        		String(L"cond_wait failed"));
                    
                } else {
                    struct timespec tmsp = timeout.getExpireTime().getTimeSpec();
                    int err = pthread_cond_timedwait(&cond,getMutex(),&tmsp);
                    if (err == ETIMEDOUT) {
                        return false;
                    }
                    if (err)
                        throw ErrNoWithDescException(THISLOCATION,err,
                        		String(L"cond_timedwait failed"));
                }
            }
            state = manualReset;
            return true;
        }

        void notify() {
            Synchronized<LLMutex> _(*this);
            if (state == false) {
                pthread_cond_signal(&cond);
                state = true;
            }            
        }

        void reset() {
            Synchronized<LLMutex> _(*this);
            state = false;
        }


    protected:
        mutable pthread_cond_t cond;
        mutable bool state;
        mutable bool manualReset;

    };


    class LLEventMutex: public LLEvent {
    public:
    	LLEventMutex(bool manualReset = false):LLEvent(manualReset) {}

    	///Perform wait on locked event (lock has been already called)
    	/**
    	 * Function can effectively unlock mutex and start to wait on event atomically
    	 * @param timeout specifies timeout
    	 * @retval true success
    	 * @retval false timeout
    	 *
    	 * @note object is locked after return
    	 */
    	bool waitLocked(Timeout timeout = Timeout())  {

				while (!state) {
					if (timeout.isInfinite()) {
						int err = pthread_cond_wait(&cond,getMutex());
						if (err)
							throw ErrNoWithDescException(THISLOCATION,err,
									String(L"cond_wait failed"));

					} else {
						struct timespec tmsp = timeout.getExpireTime().getTimeSpec();
						int err = pthread_cond_timedwait(&cond,getMutex(),&tmsp);
						if (err == ETIMEDOUT) {
							return false;
						}
						if (err)
							throw ErrNoWithDescException(THISLOCATION,err,
									String(L"cond_timedwait failed"));
					}
				}
				state = manualReset;
				return true;
			}
    };
}






#endif /*EVENT_H_*/
#endif
