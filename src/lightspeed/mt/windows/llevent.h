#pragma once
#include "../../base/windows/winpch.h"



namespace LightSpeed {


    typedef natural TimeSpec; 


    class LLEvent {
    public:

        LLEvent(bool manualReset = false):ev(CreateEvent(0,manualReset,0,0)) {}

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
            //store handle
            HANDLE tmp = ev;
            //fill handle with zeroes preventing other threads to trying get the
            //the object before it is destroyed
            ev = 0;
            //try to wait on self with timeout 0, if fails, waiting queue is not empty
            while (WaitForSingleObject(tmp,0) == WAIT_TIMEOUT)
                //notify waiting thread and try wait again
                SetEvent(tmp);
            //if we pass this loop, there should be n
            //close handle of waiting object
            CloseHandle(tmp);
        }

        bool wait(Timeout timeout = Timeout()) {
            SysTime remain = timeout.getRemain();                
            int res = WaitForSingleObject(ev,(DWORD)remain.msecs());
            if (res == WAIT_TIMEOUT) return false;
            else return true;
        }

        void notify() {
            SetEvent(ev);
        }

        void reset() {
            ResetEvent(ev);
        }


    protected:
        HANDLE ev;

    };

}



