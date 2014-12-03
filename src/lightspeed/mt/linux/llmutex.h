
#ifndef LIGHTSPEED_LINUX_MUTEXIMPL_H_
#define LIGHTSPEED_LINUX_MUTEXIMPL_H_
#include <pthread.h>
#include "../../base/exceptions/systemException.h"
#include "../../mt/timeout.h"



namespace LightSpeed {


    class LLMutex {

    public:

        LLMutex() {
        	initMutex();
        }
        ~LLMutex() {
                pthread_mutex_destroy(&mutex);
        }
        void lock() const {
                int err = pthread_mutex_lock(&mutex);
                if (err) 
                    throw ErrNoWithDescException(THISLOCATION,err,
                    		String(L"LLMutex lock failed"));
        }
        void unlock() const {
                int err = pthread_mutex_unlock(&mutex);
                if (err) 
                    throw ErrNoWithDescException(THISLOCATION,err,
                    		String(L"LLMutex unlock failed"));
                
        }
        bool tryLock() const {
                int err = pthread_mutex_trylock(&mutex);
                if (err == EBUSY) return false;
                if (err) 
                    throw ErrNoWithDescException(THISLOCATION,err,
                    		String(L"LLMutex trylock failed"));
                return true;
        }

        bool lock(const Timeout &tm) const {
        	struct timespec tmsp = tm.getExpireTime().getTimeSpec();
            int err = pthread_mutex_timedlock(&mutex,&tmsp);
            if (err == ETIMEDOUT) return false;
            if (err)
                throw ErrNoWithDescException(THISLOCATION,err,
                		String(L"LLMutex trylock failed"));
            return true;

        }

        LLMutex(const LLMutex &) {
        	initMutex();
        }
    
    protected:

        mutable pthread_mutex_t mutex;
        pthread_mutex_t *getMutex() const {return &mutex;}

        void initMutex() {
			pthread_mutexattr_t attr;
			int err;
			err = pthread_mutexattr_init(&attr);
			if (err)
				throw ErrNoWithDescException(THISLOCATION,err,
						String(L"LLMutex construction (init attr)"));
			try {
				err = pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
				if (err)
					throw ErrNoWithDescException(THISLOCATION,err,
							String(L"LLMutex construction (set attr)"));
				err = pthread_mutex_init(&mutex,&attr);
				if (err)
					throw ErrNoWithDescException(THISLOCATION,err,
							String(L"LLMutex construction (init mutex)"));
			} catch (...) {
				pthread_mutexattr_destroy(&attr);
				throw;
			}
			pthread_mutexattr_destroy(&attr);
        }



    };


}

#endif

