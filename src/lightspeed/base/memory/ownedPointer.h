/*
 * ownedPointer.h
 *
 *  Created on: 26.9.2009
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MEMORY_OWNEDPOINTER_H_
#define LIGHTSPEED_MEMORY_OWNEDPOINTER_H_

#include "allocPointer.h"

namespace LightSpeed {


	///OwnedPtr is Pointer which allows to track ownership between references
	/**
	 * @param T type of object
	 * @param ReleaseRules how object must be released
	 *
	 * Ownership is stored as bool variable of each reference. Only one reference
	 * can have ownership, others has false in this field. Ownership
	 * can be transfered between references. When reference with
	 * ownership is destroyed, object is also destroyed.
	 *
	 * Main benefit of this class is MT safety, because it doesn't need
	 * to have shared variables. If you givin object to the another
	 * thread, you can choose at the place, if you giving it with
	 * the ownership or without.
	 *
	 * Ownership is always transfered from original to copy using
	 * copy constructor, because creating a copy of objects is often way
	 * how objects are detached from original reference for keeping
	 * them for long time.
	 *
	 * Ownership is not transfered using assignment operator. This
	 * operator is often used to assign into temporary variables
	 *
	 * If you want to take ownership from another pointer, make
	 * copy using copy constructor, or use function take()
	 *
	 * If you want to construct pointer without taking ownership,
	 * use constructor with NotOwned enumeration type
	 *
	 * If original pointer has not ownership, there is no
	 * way how to get ownership.
	 */
	template<typename T, typename Allocator = StdFactory::Factory<T> >
	class OwnedPtr: public AllocPointer<T,Allocator>
	{
	public:
	    typedef AllocPointer<T,Allocator> Super;

		enum NotOwned {notOwned};

		OwnedPtr():Super() {}
		OwnedPtr(NullType x):Super(x) {}
		OwnedPtr(T *p):Super(p),owned(true) {}

		OwnedPtr(const Allocator &alloc):Super(alloc) {}
		OwnedPtr(T *p, const Allocator &alloc):Super(p,alloc) {}


        ~OwnedPtr() {
			if (owned == false) Super::detach();
		}

		///Detaches pointer and removes ownership
		/**
		 * @return pointer
		 */
		T *detach() {
			owned = false;
			return this->ptr;
		}

		///Tests ownership
		/**
		 * @retval true pointer is owned
		 * @retval false pointer is not owned
		 */
		bool isOwned() const {return owned;}

		///Constructs copy and takes ownership
		/**
		 * @param p original pointer, Ownership is given only when
		 * original pointer has ownership
		 */
		OwnedPtr(const OwnedPtr &p)
            :Super(p.get(),p.getAllocator()),owned(p.owned) {
                const_cast<OwnedPtr *>(&p)->owned = false;
		}
		///Constructs copy but do not take ownership
		/**
		 * @param p original pointer, Ownership is not give
		 */
		OwnedPtr(const OwnedPtr &p, NotOwned)
            :Super(p.get(),p.getAllocator()),owned(false) {
		}

		///Assign value, do not take ownership
		/**Assign new value, but will not get ownership. If
		 * contains owned object, it is released
		 *
		 * @param other original pointer
		 * @return this object
		 *
		 * @note assigne to itself is ignored. Assign to pointer with the
		 * same value is also ignored, because you cannot move ownership and
		 * the target cannot lost ownership when assigning pointer to the same
		 * object that has not ownership
		 */
		OwnedPtr& operator=(const OwnedPtr &other) {
			if (other.ptr != this->ptr) {
			    if (owned) Super::clear();
			    this->ptr = other.get();
			    Super::setAllocator(other);
				owned = false;
			}
			return *this;
		}

		///Takes ownership from another pointer
		/**
		 * @param p pointer, from which to take owner.
		 *
		 * Note, owner ship is moved only source object has it. If target
		 * object contains pointer to different object, it is released first
		 */
		OwnedPtr& operator<<= (const OwnedPtr &p) {
		    //make assignemnt if necessery
		    if (p.ptr != this->ptr) {
		        this->operator= (p);
		    }
		    //Only when I am not owner of pointer
		    if (!owned) {
		        //move ownership
		        owned = p.owned;
		        const_cast<OwnedPtr &>(p).owned = false;
		    }
		    //return this
			return *this;
		}

		///Gives ownership to the another object
		/**
		 * @param p target pointer that receives ownership
		 * @return refernce to me
		 */
        OwnedPtr& operator>>= (OwnedPtr &p)  {
            p <<= (*this);
            return *this;
        }

        ///Takes ownership from another pointer
        /**
         * @param p pointer, from which to take owner.
         */
        const OwnedPtr& operator<<= (const OwnedPtr &p) const {
            if (p.ptr == this->ptr && !p.owned) {
                owned = p.owned;;
                const_cast<OwnedPtr *>(p)->owned = false;
            }
            return *this;
        }

        const OwnedPtr& operator>>= (const OwnedPtr &p) const {
            p <<= (*this);
            return *this;
        }

		void swap(OwnedPtr &other) {
		    std::swap(static_cast<Super &>(*this),
                      static_cast<Super &>(other));
		    std::swap(owned, other.owned);
		}

	protected: 

		bool owned;
	};

}

namespace std {
    template<typename A, typename B>
    void swap(::LightSpeed::OwnedPtr<A,B> &a, ::LightSpeed::OwnedPtr<A,B> &b) {
        a.swap(b);
    }
}

#endif /* OWNEDPOINTER_H_ */
