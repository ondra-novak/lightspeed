#ifndef LIGHTSPEED_LINUX_THREADID_H_
#define LIGHTSPEED_LINUX_THREADID_H_

#include <pthread.h>
#include "atomic_type.h"

namespace LightSpeed {
    

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
        
        pthread_t id;
    public:

        ///constructs thread id using posix pthread identifier
        /** This constructor can be used to construct ThreadId using AtomicThreadId type
         */
        ThreadId(pthread_t id):id(id) {}
        ///constructs empty identifier
        ThreadId(NullType):id(0) {}
        
        
        ///retrieves current id as posix pthread identifier
        pthread_t get() const {return id;}
        
        ///Returns value usable for AtomicThreadId type
        /** @note return don't need to be AtomicThreadId type. It can
         * be any type that is accepted by this type. Implementation gives
         * preference to type used as value parameter in member functions, for
         * example type used as value in exchange() function
         */
        atomicValue asAtomic() const {return (atomic)id;}
        
        ///Retrieves current thread identifier
        static ThreadId current() {
            return ThreadId(pthread_self());
        }
        
        ///returns true, if two identifiers are equal
        bool operator==(const ThreadId &other) const {
            return pthread_equal(id, other.id) != 0;
        }
        ///returns true, if two identifiers are not equal
        bool operator!=(const ThreadId &other) const {
            return pthread_equal(id, other.id) == 0;
        }
        ///tests, whether identifier is nil
        bool operator==(NullType) const {
            return id == 0;
        }
        ///tests, whether identifier is not nil
        bool operator!=(NullType) const {
            return id != 0;
        }
        
    };
/*    
    
    template<typename T>
    class ToAsciiFunctor;
    template<typename T, natural base>
    struct ToAsciiFunctorIntegers;

    template<>
    class ToAsciiFunctor<ThreadId>: public ToAsciiFunctorIntegers<pthread_t,16> {              
    public:
        template<typename CharType>
        void operator()(const ThreadId &value, Buffer<CharType> &buff) const {
        	pthread_t val = value.get();
            return ToAsciiFunctorIntegers<pthread_t,16>::operator()(val,buff);
        }

        template<typename CharType>
        ThreadId operator()(const CharType *buffer) const {
        	pthread_t val = ToAsciiFunctorIntegers<pthread_t,16>::operator()(buffer);
            return ThreadId(val);
        }

    };
  */  
}

#endif /*THREADID_H_*/
