#ifndef LIGHTSPEED_MEMORY_REFCNTPTR_H_
#define LIGHTSPEED_MEMORY_REFCNTPTR_H_

#include "allocPointer.h"
#include "../debug/break.h"
#include "../export.h"


namespace LightSpeed {



	///Implements ref counting to allow RefCntPtr work with the object
	/**
	 * @param Counter type that implements counting
	 */
	class RefCntObj {
		static const atomicValue fastFlag = (atomicValue)((~natural(0)) << (sizeof(natural) * 8 - 1));
		static LIGHTSPEED_EXPORT atomicValue initialValue;
	public:

		RefCntObj():counter(initialValue) {}

		///adds reference to counter
		void addRef() const;

		///releases reference to counter
		/**
		 * @retval true counter reaches a zero and object must be destroyed
		 * @retval false counter did not reached a zero
		 */
		bool release() const;

		///Returns true, if object is not shared
		bool isShared() const {return (counter & ~fastFlag) != 1;}

		///Sets counter to MT
		/** Function is MT safe and can be called even if object is already MT aware. In this case
		 * function does nothing rather of trying to re-enable MT access
		 */
		void enableMTAccess() const ;/*{counter &= ~fastFlag;}*/
		///Sets counter to ST
		/** Function is MT safe. However you should ensure, that after disabling MT, object cannot be
		 * accessed from other thread otherwise a race condition can happen.
		 */
		void disableMTAccess() const ;/*{counter |= fastFlag;}*/
		///Object is allocated statically
		/** allows to work with RefCntObj, but allocated statically.
		 * Counter should never reach zero
		 */
		void setStaticObj() const {addRef();}

		bool isMTAccessEnabled() const {return (counter & fastFlag) == 0;}

		void debugClear() const;

		///enables MT Access for all objects using RefCntObj as base class created from now
		static void globalEnableMTAccess();
		///disable MT Access for all objects using RefCntObj as base class created from now
		static void globalDisableMTAccess();



	protected:
		///counter
		mutable atomic counter;

		void addRefMT() const;
		bool releaseMT() const;
	};


    ///Shared ref count pointer (intrusive)
    /**Sharing is implemented intrusive using internal counter and it is controlled
     * by methods addRef() and release()
     * 
     * Deleting of controlling object is handled by this class instead of
     * referenced class. The referenced class must only export the methods 
     * addRef() and release() and must be able report, that reference count
     * reached zero. 
     */
    template<typename T,  typename Alloc = StdFactory::Factory<T> >
    class RefCntPtr: public AllocPointer<T,Alloc> {
    public:
        typedef AllocPointer<T,Alloc> Super;

        ///Constructs empty pointer
        RefCntPtr() {}


        ///Constructs empty pointer
        RefCntPtr(NullType x):Super(x) {}



        ///Constructs a pointer and add reference
        /**
         * @para p pointer to assign the new instance
         */ 
       RefCntPtr(T *p):Super(p) {
            addRefPtr();
        }

       ///Conversion from wrapped pointer
       /**
        * @param p
        *
        * @note GCC requieres const-reference here. Clang will compile without it.
        */
        RefCntPtr(const Pointer<T> &p):Super(p) {
        	addRefPtr();
        }
        ///Copy constructor
        /**@param other source object
         */
        RefCntPtr(const RefCntPtr &other)
            :Super(other.get(),(const Alloc &)other) {
            addRefPtr();
        }




        ///Destroy instance and release reference
        /**
         * Destroys instance and releases reference. Object can be
         * released, if reference count reaches zero
         */
        ~RefCntPtr() {
            releasePtr();
        }



        operator RefCntPtr<const T,Alloc>() const {
        	return RefCntPtr<const T,Alloc>(this->ptr,*this);
        }


        ///Assignment
        /**@param other source object
         * @return this         
         */
        RefCntPtr &operator=(const RefCntPtr &other) {
        	Alloc::operator=(other);
        	other.manualAddRef();
        	releasePtr();
        	this->ptr = other.ptr;
            return *this;
        }

		//C++x11
#ifdef LIGHTSPEED_ENABLE_CPP11

		RefCntPtr(RefCntPtr &&other) {
			this->ptr = other.ptr;
			other.ptr = 0;
		}
		RefCntPtr &operator=(RefCntPtr &&other) {
			if (this != &other) {	
				releasePtr();
				this->ptr = other.ptr;
				other.ptr = 0;
			}
			return *this;
		}

#endif
        ///Retrieves MT version of ref.counter
        /**
         * By default, every RefCntPtr is constructed for Single-threaded
         * environment. You can uses RefCntPtr in this mode in MT environment,
         * but you should not share such an objects between the threads.
         *
         * This function enables MT-sharing for current object. You can
         * then duplicate pointers and share object independently to
         * on which thread this happen. Note that sharing object between
         * the threads is slightly slower. But sharing pointer between
         * threads without calling this function can cause unpredictable
         * results,
         *
         * @return copy of reference initializes for multithreaded access
         *
         * @note You should always use result to multithreading share.
         * Do not rely on implementation, it can be changed in the future.
         * Always use getMT() to share object between the threads, even if you
         * know, that object is already in MT mode. There can be change in the future
         * while multiple counters will be used each for different thread
         *
         */
		RefCntPtr getMT() const {
			if (this->ptr) {
				const RefCntObj *obj = static_cast<const RefCntObj *>(this->ptr);
				obj->enableMTAccess();
			}
			return *this;
		}

		///True, if object is shared at more than once
		/**
		 * @retval true object is shared
		 * @retval false object is not shared, this is only reference
		 */
		bool isShared() const {
			if (this->ptr == 0) return false;
			return this->ptr->isShared();
		}

		///Manually adds reference
		/** Use only in special cases, when you track count of references differently.
		  Note that mistake in reference counting may cause memory leak or
		  working with destroyed object
		  */
		void manualAddRef() const {
			addRefPtr();
		}
		///Manually releases reference
		/** Use only in special cases, when you track count of references differently.
		  Note that mistake in reference counting may cause memory leak or
		  working with destroyed object
		  */
		bool manualRelease() const {
			return releasePtrNoDetach();
		}

		///Detaches object from the reference counting
		/** Use only if you want to stop reference counting on the
			object and use another type of reference tracking. This
			is also useful, when you need to change implementation in the
			counting for example from MT unsafe to MT save.

			Function decreases reference and removes pointer. Also sets 
			this smart pointer to NULL. It expects, that reference count
			reaches zero, but object will not destroyed. This expectation
			is not checked, so you can call this function even the object
			is still shared. But this is not recommended, because this
			will not prevent destruction later. You should do this only
			when isShared() returns false
			*/
		T *detach() {
		    if (this->ptr) {
		        this->ptr->release();
		    }
			return Super::detach();
		}

		///Attaches object to the pointer
		void attach(T *ptr) {
		    Super::attach(ptr);
		    addRefPtr();
		}

		///Serializing support
		template<typename Arch> 
		void serialize(Arch &arch) {
			if (arch.loading()) {
				releasePtr();
				Super::serialize(arch);
				addRefPtr();
			} else {
				Super::serialize(arch);
			}			
		}

		///Creates new instance using current factory
		void create() {Super::create();addRefPtr();}
		///Creates new instance using current factory
		/**
		 * @param x source instance
		 */
		void create(const T &x) {Super::create(x);addRefPtr();}


		void clear() {
			releasePtr();
		}
    protected:


		///safety add new reference
        inline void addRefPtr() const {
            if (this->ptr) {
            	this->ptr->addRef();
            }
        }
        ///safety release reference and handle releasing the resource
        inline bool releasePtr() const {
            if (!this->ptr) return false;
            if (this->ptr->release() == true) {
#ifdef _DEBUG
            	this->ptr->debugClear();
#endif
                const_cast<RefCntPtr *>(this)->Super::clear();
            } else {
                const_cast<RefCntPtr *>(this)->Super::detach();
            }
			return true;
        }

        ///releases reference, but will not detach the object from the pointer
		inline bool releasePtrNoDetach() const {
			if (!this->ptr) return false;
			if (this->ptr->release() == true) {
				const_cast<RefCntPtr *>(this)->Super::clear();
			}
			return true;
		}
};





    

} // namespace LightSpeed

#endif /*REFCNTPTR_H_*/
