#ifndef LIGHTSPEED_SHAREDPTR_H_
#define LIGHTSPEED_SHAREDPTR_H_

#include "sharedResource.h"
#include "allocPointer.h"


namespace LightSpeed {

    template<class T, typename Allocator = StdFactory::Factory<T> >
    class SharedPtr:public SharedResource, public AllocPointer<T,Allocator> {
    public:
    	typedef AllocPointer<T,Allocator> Super;
        SharedPtr() {}
        SharedPtr(T *ptr):Super(ptr) {}
        SharedPtr(NullType x):Super(x) {}

        SharedPtr(const Allocator &r):Super(r) {}
        SharedPtr(T *ptr,const Allocator &r):Super(ptr,r) {}

        SharedPtr(const SharedPtr &other)
			:SharedResource(other), Super(other.get(),other) {}

        SharedPtr &operator=(const SharedPtr<T,Allocator> &other) {
        	if (this == &other) return *this;
        	//two objects refers the same pointer!
        	if (this->ptr == other.ptr) {
        		//if this object is alone (or both)
        		if (!this->isShared()) {
        			//join this object to other's chain
        			//redirect next (which points to me) to other's next
        			this->next = other.next;
        			//redirect other's next this thi
        			other.next = this;
        			//joined

        		//if other object is alone
        		} else if (!other.isShared()) {
        			//join other object to this chain
        			//redirect other's next (which points to self) to my next
        			other.next = this->next;
        			//redirect my next to other
        			this->next = &other;
        		} else {
        			//boths contains chain - this situation should not appear, but must be solved if does
        			//first we should check whole chain if both sides are already included
        			const SharedPtr *q = this;
        			//process whole chain until q->next points to this
        			while (q->next != this) {
        				//we did find other side in the chain
        				if (*q == other)
        					//this is null operation - already done
        					return *this;
        				else
        					//continue to search
        					q = static_cast<const SharedPtr *>(q->next);
        			}

        			//if we did not find other side, join two chains into one
        			//redirect q's next (points to this) to other's next
        			q->next = other.next;
        			//redirect other's next to this
        			other.next = this;
        		}
        		//in all cases, return this
        		return *this;
        	} else {
        		//in case two different pointers, perform standard assignment
        		return SharedResource::makeAssign(other);
        	}
        }

        void redirect(T *other) {
            this->ptr = other;
            SharedResource::forEachInstance(updateAfterRedirect);
        }

        ~SharedPtr() {
            if (SharedResource::isShared()) {
            	this->ptr = 0;
            }
        }


    protected:

        static bool updateAfterRedirect(SharedResource *cur,
            SharedResource *src) {
                static_cast<SharedPtr<T> *>(cur)->ptr 
                        = static_cast<SharedPtr<T> *>(src)->ptr; 
            return true;
        }
    };


    ///Weak references
    /** Helps to implement weak references. Object that must be referenced by a weak reference must
     * include instance of this class as member variable. Variable is initialized with pointer
     * to the object. To create weak reference, just simply use this instance to create copy
     * of SharedPtr instance. The new instance will contain pointer to the object and will be
     * connected with the WeakRefTarget. Other instances can be created by copying the SharedPtr instances
     * as long as needed. All instances are connected to each other as SharedPtr is implemented.
     *
     * Once the object is destroyed, the destructor of the WeakRefTarget sets all pointers to NULL.
     *
     * @note Neither WeakRefTarget nor SharedPtr are MT safe. You have always to use proper synchronization or
     * ensure, that weak references are accessed from one thread at time only.
     */
    template<typename T>
    class WeakRefTarget: public SharedPtr<T > {
    public:

    	WeakRefTarget(T *ptr):SharedPtr<T>(ptr) {}
    	~WeakRefTarget() {
    		this->redirect(0);
    	}

    };

    ///Keeps pointer to static object to allow be shared, but doesn't actually destroys it
    /**
     * If you need to share static object where sharing using SharePtr is
     * used, you can declare this object and use pointer to static object
     * for initialization. Then you can create many shares as you want.
     *
     * SharedStaticPtr can be used only once in sharing circle. Once
     * SharedStaticPtr instance is destroyed, it redirects other
     * instances to NULL that guards static object to be destroyed.
     *
     * @tparam T pointer type
     * @tparam Allocator even if this object doesn't need alocator, you
     * have to specify the same allocator like allocator used on apropriate
     * SharedPtr that you want to use, otherwise instances will be incompatible
     */
    template<typename T, typename Allocator = StdFactory::Factory<T> >
    class SharedStaticPtr: public SharedPtr<T,Allocator> {
    public:
    	typedef SharedPtr<T,Allocator> Super;
    	SharedStaticPtr() {}
    	SharedStaticPtr(T *ptr):Super(ptr) {}
    	SharedStaticPtr(NullType x):Super(x) {}
    	SharedStaticPtr(T *ptr,const Allocator &r):Super(ptr,r) {}

    	~SharedStaticPtr() {
    		this->redirect(0);
    	}

    	SharedStaticPtr &operator=(T *other) {
    		redirect(other);
    		return *this;
    	}

    protected:
    	SharedStaticPtr(const SharedStaticPtr &other);
    	SharedStaticPtr &operator=(const SharedStaticPtr<T,Allocator> &other);

    };

}

#endif
