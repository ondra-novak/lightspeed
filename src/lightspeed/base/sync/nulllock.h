/*
 * $Id: nulllock.h 4572 2014-02-08 21:55:48Z ondrej.novak $
 */


#ifndef _LIGHTSPEED_SYNC_NULLLOCK
#define _LIGHTSPEED_SYNC_NULLLOCK


#pragma once 


namespace LightSpeed {
    ///Default lock class
    /** Null lock is empty class, that contains all important methods that must be
    implemented in common lock. Unlike other locks, NullLock is doesn't provide
    any action. All functions all empty. You can use NullLock as template
    argument, if template requests you for a lock class. With NullLock, you will
    create the fastest implementation, but with no MT safety.

    NullLock is also good choice for default template arguments, if you want
    to specify, that class needs exclusive lock, but you don't want to include
    a multithread library.
    */
    class NullLock
    {
    public:
        ///Locks the  instance
        /**
        Implementation of this class is empty.
        */
        void lock()  {}
        ///Unlocks the  instance
        /**
        Implementation of this class is empty.
        */
        void unlock()  {}

        ///Try locks the instance
        /**
        @retval true when success
        @retval false when failed
        @note Implementation of that function is empty in the NullLock class
        */
        bool tryLock()  {return true;}

		///Locks the instance specifying timeout to wait
		template<typename TimeoutSpec>
		bool lock(const TimeoutSpec &)  {return true;}
    };


}
#endif

