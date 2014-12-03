#ifndef LIGHTSPEED_SHAREDRESOURCE_H_
#define LIGHTSPEED_SHAREDRESOURCE_H_

#include "../debug/break.h"
#include "../types.h"

#ifdef _WIN32
#pragma warning( disable: 4355 )
#endif

namespace LightSpeed {
	
	///Class implements the simplest resource sharing.
	/**Class is best if used to track sharing of small data. Sharing itself is implemented by
	 * copiing, but class can track instances that has been copied from one source. The best usage
	 * is to implement various wrappers to the operation system resources, for example file descriptors, 
	 * sockets, or Windows handles. Every instance that derives this class will implement copy constructor
	 * by simle copying the handle. Class SharedResource can later reply to question "is there any instance
	 * that contains the same handle?". If reply is no, defived class can close the handle if it is no
	 * longer needed. In positive question, it cannot close handle, because there is another instance
	 * that needs this handle.
	 * 
	 * SharedResource doesn't uses counting references. Its creates enclosed linked list (ring) that
	 * contains all shared instances. This simplyfies implementation, because no memory allocation is
	 * needed. But if resource is shared in many copies, it can slow down the access, especially 
	 * destruction (because it needs to walk through the whole list and find previous item. You should
	 * not use this class to share instance in thousands copies.
	 * 
	 * @note class is not MT safe
	 */
    class SharedResource {
    public:
        

    	///Constructs new alone instance ready for sharing
        SharedResource():next(this) {}
        
        ///Creates new instance from another instance
        /**
         * To track alive instances, new instance is added after 'other' redirecting
         * next pointer to itself a settings itself next to nexet item after other
         * 
         * @param other source instance
         * @note this operation has complexity: O(1)
         */
        SharedResource(const SharedResource &other):next(other.next) {
            other.next = this;
        }

        ///Creates new instance from another instance in MT environment
        /**
         * This constructor must be called in order to create shared
         * instance in MT environment. It uses lock to make exclusive access
         * to the pointers
         *
         * @param other source object to share
         * @param lk lock used obtain exclusive access. Note that lock
         * must be also shared (cannot have standalone lock for every instance.
         * If lk is NULL, locking is s skipped
         *
         * @see unshare
         */
        template<typename Lock>
        SharedResource(const SharedResource &other, Lock *lk) {
        	if (lk) {
        		lk->lock();
            	next = other.next;
            	other.next = this;
            	lk->unlock();
        	} else {
            	next = other.next;
            	other.next = this;
        	}
        }

        ///Creates new instance from another instance using the assignment operator
        /**
         * 
         * First removes itself from existing ring and then adds itself to the ring where other belongs
         * to. 
         * 
         * @param other source instance
         * @return this reference
         * @note this operation has complexity: O(n) for remove from existing ring, O(1) otherwise
         */
        SharedResource &operator=(const SharedResource &other) {        	
            removeItself();
            next = other.next;
            other.next = this;
            return *this;
            
        }

        ///Removes itself from the ring
        /**
         * @note this operation has complexity: O(n)
         */
        ~SharedResource() {
            removeItself();
        }
        
        
        ///Retrieves, whether resource is shared or alone
        /**
         * @retval true there is another instance that shares data.
         * @retval false there are no other instances, this instance is alone
         * @note operation has complexity: O(1)
         */
        bool isShared() const {
            return next != this;
        }
        
        ///Counts of shared instances
        /**
         * @return count of shared instances
         * @note this operation has complexity: O(n)
         */
        natural count() const {
        	natural cnt = 0;
        	const SharedResource *cur = this;
        	do { cnt++;
        		 cur = cur->next;
        	} while (cur != this);
        	return cnt;
        		
        }
        
        
        ///Removes sharing in MT environmnent
        /** Because destructor of SharedResource cannot work with
         * lock defined in derived class, destructor of derived
         * class which defines the lock must call this function
         * to ensure, that object will not be shared before it is destroyed
         *
         * Leave object shared (connected to sharing ring) may cause
         * conflict while item is removed from the ring. After unshare()
         * object is removed and so it cannot make conflict.
         *
         * @param lk Pointer to lock, it can be NULL to provide unshare without lock
         *
         * @note you can call this function repeatedly. First call unshares, other
         * does nothing
         */
        template<typename Lock>
        void unshare(Lock *lk) {
        	if (lk) {
        		lk->lock();
        		removeItself();
        		lk->unlock();
        	} else {
        		removeItself();
        	}
        }


    protected:
        
    	///Contains pointer to next instance. It will never contain NULL    	
    	mutable const SharedResource *next;
    	
		void removeItself() const {
        	const SharedResource *cur = this;        	        
        	while (cur->next != this) 
        		cur = cur -> next;        		        	
        	cur->next = next;
        	next = this;
        }        
        
        ///For each instance in the ring calls the functor
        /**
         * Functor is called for all instances, includin this. Function excepts functor with 
         * twp parameter - pointer to instance (SharedResource) and pointer to caller instance. 
         * Function excepts, that functor will change the first instance. 
         * The best usage is to update state in all shared instances
         * @param fn functor or function. It should return true to continue or false to exit
         * @retval true successfully done
         * @retval false functor returned false
         */
        template<class Functor>
        bool forEachInstance(Functor fn) {
        	const SharedResource *cur = this;
        	do { if (!fn(const_cast<SharedResource *>(cur),this))
        			return false;
        		cur = cur -> next;        		
        	} while (cur != this);
        	return true;        	
        }
        
        
		///Helps to implement assignment operator
		/**
		 * if it called from derived class, it makes inplace destruction
		 * recreation using the copy constructor. 
		 *
		 * @param other object to assign
		 *
		 * @note Don't use 'other' of different type then this. If you
		 * cannot handle this, use proper cast before argument is passed
		 * to the function. Because class is not dynamic, it cannot
		 * check type of this pointer. It will always to use static_cast
		 * to cast this pointer of type SharedResource to target type
		 */
		template<class T>
		T &makeAssign(const T &other) {			
			if (&other == this) return *static_cast<T *>(this);
			T *t = static_cast<T *>(this);
			t->~T();
			new(t) T(other);
			return *t;
		}


        ///For each instance in the ring calls the functor
        /**
         * Functor is called for all instances, includin this. Function excepts functor with 
         * one parameter - const pointer to instance (SharedResource). Function excepts, that functor will 
         * NOT change the instance. 
         * @param fn functor or function. It should return true to continue or false to exit
         * @retval true successfully done
         * @retval false functor returned false
         */

        template<class Functor>
        bool forEachInstance(Functor fn) const {
        	const SharedResource *cur = this;
        	do { if (!fn(cur,this)) return false;
        		cur = cur -> next;        		
        	} while (cur != this);
        	return true;        	
        }

        
        void shareWith(SharedResource *to) const {
        	removeItself();
        	next = to->next;
        	to->next = this;
        }

		void merge( const SharedResource & other ) const 
		{
			const SharedResource *nx = other.next;
			other.next = this;
			const SharedResource *k = this;
			while (k->next != this) k = k ->next;
			k->next = nx;
		}

	};
}

#endif /*SHAREDRESOURCE_H_*/
