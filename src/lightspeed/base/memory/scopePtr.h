/*
 * scopePtr.h
 *
 *  Created on: 3.9.2009
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_MEMORY_SCOPEPTR_H_
#define _LIGHTSPEED_MEMORY_SCOPEPTR_H_

#include "allocPointer.h"

namespace LightSpeed {

	///ScopePtr
	template<class T, class Allocator = StdFactory::Factory<T> >
	class ScopePtr: public AllocPointer<T,Allocator> {
	public:

		typedef AllocPointer<T,Allocator> Super;
       ///Constructs empty pointer using allocator's default constructor
       ScopePtr() {}
       ScopePtr(NullType x):Super(x) {}
       ScopePtr(const Allocator &alloc):Super(alloc) {}
       explicit ScopePtr(T *ptr): Super(ptr) {}
       ScopePtr(T *ptr, const Allocator &alloc): ScopePtr(ptr,alloc) {}
	   
       ScopePtr &operator=(const ScopePtr &other) {
    	   Super::operator =(other);
    	   return *this;
       }

	   void attach(T *ptr) {
		   if (this->ptr) this->clear();
		   this->ptr = ptr;
	   }
	   
	};

}

namespace std {

	template<class T, class Allocator>
	void swap(LightSpeed::ScopePtr<T,Allocator> &a, LightSpeed::ScopePtr<T,Allocator> &b) {
		a.swap(b);
	}

}


#if 0
    template<class T, template<class> class ReleaseRules = ReleaseDeleteRule>
    class ScopePtr: public Pointer<T> {
    public:
        ScopePtr() {}
        ScopePtr(NullType x):Pointer<T>(x) {}
        ScopePtr(T *x):Pointer<T>(x) {}
        ///Copy constructor will initialize this pointer to NULL
        /**
         * This behaviour protects copied pointers to be taken on by other
         * objects through default copy constructor. ScopePtr guards its
         * pointer and will not give ownership to the anothed object
         * @param other source object (ignored)
         */
        ScopePtr(const ScopePtr &other):Pointer<T>(nil) {}


        ScopePtr &operator=(NullType x) {
            if (this->ptr) {
                ReleaseRules<T> release;
                release(this->ptr);
            }
            this->ptr = 0;
            return *this;
        }

        ScopePtr &operator=(T *x) {
            if (this->ptr) {
                ReleaseRules<T> release;
                release(this->ptr);
            }
            this->ptr = x;
            return *this;
        }

        ~ScopePtr() {
            (*this) = nil;
        }
    };

}
#endif
#endif /* _LIGHTSPEED_SCOPEPTR_H_ */
