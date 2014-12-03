#ifndef LIGHTSPEED_WINDOWS_THREADID_H_
#define LIGHTSPEED_WINDOWS_THREADID_H_

#define WIN32_LEAN_AND_MEAN
#include "../../base/windows/winpch.h"
#include "../atomic.h"

namespace LightSpeed {

#pragma warning(disable:4197)
    

    ///Declaration of type atomic variable that can hold thread id
    /** Use this type to store thread Id as atomic type. You can use
     * all atomic operations. 
     * 
     * This type garantees, that there will be no more threads with the
     * same identifier, but there can be more identifiers for one thread. To
     * successfully use compareAndExchange operation, you have to compare
     * result of this operation using the ThreadId's compare operator. In case
     * that you expect same thread identifier, and compare operator returns
     * true, you should update 'test' variable with value returned by compareAndExchange
     * and repeate this operation.
     */
    typedef atomic AtomicThreadId;
    ///Identify the thread
    /** Using this class, you can simple identify the thread. 
     * The instance of this class holds platform specific thread identifier
     * Portable usage of this class is storing and comparing the identifier
     * 
     * To retrieve current thread identifier, use ThreadId::current() function.
     * Then you can store the identifier in the variable and you can later
     * compare this identifier with another identifier
     */
    class ThreadId {
        
        DWORD id;
    public:
        ///constructs thread id using posix pthread identifier
        /** This constructor can be used to construct ThreadId using AtomicThreadId type
         */
        ThreadId(DWORD id):id(id) {}
        ///constructs empty identifier
        ThreadId(NullType x):id(0) {}
        
        
        ///retrieves current id as posix pthread identifier
        DWORD get() const {return id;}
        
        ///Returns value usable for AtomicThreadId type
        /** @note return don't need to be AtomicThreadId type. It can
         * be any type that is accepted by this type. Implementation gives
         * preference to type used as value parameter in member functions, for
         * example type used as value in exchange() function
         */
        AtomicThreadId asAtomic() const {return AtomicThreadId(id);}
        
        ///Retrieves current thread identifier
        static ThreadId current() {
            return ThreadId(GetCurrentThreadId());
        }
        
        ///returns true, if two identifiers are equal
        bool operator==(const ThreadId &other) const {
            return other.id == id;
        }
        ///returns true, if two identifiers are not equal
        bool operator!=(const ThreadId &other) const {
            return other.id != id;
        }
        ///tests, whether identifier is nil
        bool operator==(NullType x) const {
            return id == 0;
        }
        ///tests, whether identifier is not nil
        bool operator!=(NullType x) const {
            return id != 0;
        }
        
    };
    
}

#endif /*THREADID_H_*/
