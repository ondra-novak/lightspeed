/*
 * pointer.h
 *
 *  Created on: 3.9.2009
 *      Author: ondra
 */


#pragma once
#ifndef _LIGHTSPEED_MEMORY_POINTER_H_
#define _LIGHTSPEED_MEMORY_POINTER_H_

#include "../types.h"
#include "../compare.h"
#ifndef LIGHTSPEED_NOCHECKPTR
#include "../debug/programlocation.h"
#endif

namespace LightSpeed {

    struct ProgramLocation;
    void throwNullPointerException(const ProgramLocation &loc);


    ///Base class for all smart-pointer implementation
    /** wraps single pointer and emulates all features of it.
     * Also handle nil as NULL pointer
     *  */
    template<class T>
    class Pointer : public ComparableLess<Pointer<T> >{
        friend class ComparableLess<Pointer<T> >;
    public:

        ///Constructs empty pointer
        Pointer():ptr(0) {}
        ///Constructs empty pointer
        Pointer(NullType ):ptr(0) {}
        ///Initializes pointer object
        /**
         * @param p pointer used to initialization
         */
        Pointer(T *p):ptr(p) {}

#ifdef LIGHTSPEED_NOCHECKPTR
#define LIGHTSPEED_POINTER_CHECK(ptr) do { if (ptr == 0) throwNullPointerException(THISLOCATION); } while (false)
#else
#define LIGHTSPEED_POINTER_CHECK(ptr)
#endif

        ///Member access
        /**
         * @return pointer to object
         * @exception NullPointerException if used with null. This feature can
         * be disabled defining preprocessor variable LIGHTSPEED_NOCHECKPTR
         */
        T *operator->() const {
            LIGHTSPEED_POINTER_CHECK(ptr);
            return ptr;
        }
        ///Dereference
        /**
         * @return reference  to object
         * @exception NullPointerException if used with null. This feature can
         * be disabled defining preprocessor variable LIGHTSPEED_NOCHECKPTR
         */
        T &operator*() const {
            LIGHTSPEED_POINTER_CHECK(ptr);
            return *ptr;
        }
        ///conversion to pointer
        /**
         * @return content of instance
         */
        operator T *() const {return ptr;}

        ///current pointer
        /**
         * @return current pointer value
         */
        T *get() const {
            return ptr;
        }
        ///current pointer with NULL checking
        /**
         * @return current pointer value
         * @exception NullPointerException if used with null. This feature cannot
         * be turned off, so it will work even if LIGHTSPEED_NOCHECKPTR is defined
         */
        T *safeGet() const {
            if (ptr == 0) throwNullPointerException(THISLOCATION);
            return ptr;
        }

        ///Test operator
        /**
         * @retval true pointer contains address
         * @retval false pointer is NULL
         */
//        operator bool() const {return ptr != 0;}

        ///Const conversion
        /**
         * converts pointer to Pointer<const T>
         */
        operator Pointer<const T>() const {return ptr;}

        ///Negative test
        /**
         * @retval true pointer contains NULL
         * @retval false pointer contains address
         */
        bool operator !() const {return ptr == 0;}
        T &operator[](int i) const {
            LIGHTSPEED_POINTER_CHECK(ptr);
            return ptr[i];
        }

        ///Allows serialization
		template<typename Arch>
		void serialize(Arch &arch) {
			arch(ptr);
		} 

		///Empty test
		/**
		 * @retval true pointer contains address
		 * @retval false pointer contains NULL
		 */
	    bool empty() const {return ptr == 0;}

	    typedef T ItemT;



    protected:
        mutable T *ptr;

        bool lessThan(const Pointer<T> &other) const {
            return ptr < other.ptr;
        }

        bool isNil() const {
            return ptr == 0;
        }
    };

#undef LIGHTSPEED_POINTER_CHECK


    template<>
    class Pointer<const void> : public ComparableLess<Pointer<const void> >{
        friend class ComparableLess<Pointer<const void> >;
    public:

        Pointer():ptr(0) {}
        Pointer(NullType ):ptr(0) {}
        Pointer(const void*p):ptr(p) {}
        operator const void*() const {return ptr;}
        const void*get() const {
            return ptr;
        }
        const void*safeGet() const {
            if (ptr == 0) throwNullPointerException(THISLOCATION);
            return ptr;
        }
        operator bool() const {return ptr != 0;}
        bool operator !() const {return ptr == 0;}
	    bool empty() const {return ptr == 0;}

	    typedef const void ItemT;

    protected:
        mutable const void *ptr;

        bool lessThan(const Pointer &other) const {
            return reinterpret_cast<const byte *>(ptr) < reinterpret_cast<const byte *>(other.ptr);
        }

        bool isNil() const {
            return ptr == 0;
        }
    };


    template<>
    class Pointer<void> : public ComparableLess<Pointer<void> >{
        friend class ComparableLess<Pointer<void> >;
    public:

        Pointer():ptr(0) {}
        Pointer(NullType ):ptr(0) {}
        Pointer(void *p):ptr(p) {}
        operator void *() const {return ptr;}
        void *get() const {
            return ptr;
        }
        void *safeGet() const {
            if (ptr == 0) throwNullPointerException(THISLOCATION);
            return ptr;
        }
        operator bool() const {return ptr != 0;}
        operator Pointer<const void>() const {return ptr;}
        bool operator !() const {return ptr == 0;}
	    bool empty() const {return ptr == 0;}

	    typedef void ItemT;

    protected:
        mutable void *ptr;

        bool lessThan(const Pointer<void> &other) const {
            return reinterpret_cast<byte *>(ptr) < reinterpret_cast<byte *>(other.ptr);
        }

        bool isNil() const {
            return ptr == 0;
        }
    };


 }


#endif /* _LIGHTSPEED_POINTER_H_ */
