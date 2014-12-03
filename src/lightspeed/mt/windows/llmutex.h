#pragma once

#include "../base/windows/winpch.h"



namespace LightSpeed {


    class LLMutex {

    public:

        LLMutex() {
            InitializeCriticalSection(&csect);
        }
        ~LLMutex() {
            DeleteCriticalSection(&csect);
        }
        void lock() const {
            EnterCriticalSection(&csect);
        }
        void unlock() const {
            LeaveCriticalSection(&csect);
        }
        void tryLock() const  {
            TryEnterCriticalSection(&csect);
        }
    
    protected:

        mutable CRITICAL_SECTION csect;

    };


}



