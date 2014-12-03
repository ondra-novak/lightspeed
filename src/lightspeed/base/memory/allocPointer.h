/*
 * dynpointer.h
 *
 *  Created on: 6.1.2010
 *      Author: ondra
 */

#pragma once
#ifndef _LIGHTSPEED_DYNPOINTER_H_
#define _LIGHTSPEED_DYNPOINTER_H_

#include "pointer.h"
#include "../containers/move.h"
#include "stdFactory.h"


namespace LightSpeed {


   ///Pointer that handles allocation and deallocation using allocator
   /** This pointer also handles allocation and ownership of object.
    *
    *  You can define allocator, that will be used for allocation. Pointer
    *  then can deallocate the object using the same allocator. Alloc
    *  is carried with pointer in the instance
    *
    *  @param T type of object (or base class)
    *  @param Alloc Type of allocator
    *
    * */
   template<class T, class Alloc = StdFactory::Factory<T> >
   class AllocPointer: public Pointer<T>, protected Alloc {
   public:

       ///Constructs empty pointer using allocator's default constructor
       AllocPointer() {}
       AllocPointer(NullType x):Pointer<T>(x) {}
       ///Constructs empty pointer and sets instance of allocator
       /**
        * @param alloc instance of allocator
        * You can use functions attach() or create() to initialize pointer
        */
       AllocPointer(NullType , const Alloc &alloc):Alloc(alloc) {}

       ///Constructs and initializes pointer and initializes allocator using default constructor
       /**
        * @param ptr new pointer. Pointer must refer object allocated using
        *       allocator defined in template argument of this class
        */
       AllocPointer(T *ptr): Pointer<T>(ptr) {}

       ///Constructs and initializes pointer and allocator instance
       /**
        *
        * @param ptr new pointer. Pointer must refer object allocated using
        *       allocator defined in template argument of this class
        * @param alloc instance of allocator. Don't need to be same instance,
        *       but must be compatible with instance that allocated the pointer.
        *       In most of cases, Alloc is defined as shared instance.
        * @return
        */
       AllocPointer(T *ptr, const Alloc &alloc): Pointer<T>(ptr),Alloc(alloc) {}

       ///Destroys pointer and deallocates object
       /** If you want to auto-deletion, use detach() before destruction
        *
        */
       ~AllocPointer() {
           clear();
       }

	   //C++x11
#if 0
	   AllocPointer(AllocPointer &&other):Pointer<T>(other) {other.ptr = 0;}
	   AllocPointer &operator=(AllocPointer &&other) {
		   if (this != &other) {
				clear();
				this->ptr = other.ptr;
				other.ptr = 0;
		   }
		   return *this;
	   }
#endif
       ///Detaches object from the smart pointer
       /**
        * Detach mean, that pointer value is removed from the instance
        * of the smart pointer and it is returned as result. This prevent
        * destructor to destroy the object
        *
        * @return pointer to object before detaching.
        */
       T *detach() {
           T *out = this->ptr;
           this->ptr = 0;
           return out;
       }

       ///Attaches pointer to the smart pointer instance
       /**
        * @param obj pointer to object. It have to be allocated by allocator
        *   compatible with allocator specified by template argument.
        */
       void attach(T *obj) {
           clear();
           this->ptr = obj;
       }

       ///Clears the pointer destroying the object that holds
       void clear() {
           if (this->ptr) {
               Alloc alloc = *this;
               T *save = this->ptr;
               this->ptr = 0;
               alloc.destroyInstance(save);
           }
       }


       ///Creates new instance of object and sets the smart pointer to the object
       /**
        * @param other source object.
        *
        * @note also perfoms clear() before creation.
        */
       void create(const T &other) {
           clear();
           Alloc &alloc = *this;
           this->ptr = alloc.createInstance(other);
       }

       ///Creates new instance of object using default constructor
       /**
        * @note also perfoms clear() before creation.
        */
       void create() {
           clear();
           Alloc &alloc = *this;
           this->ptr = alloc.createInstance();
       }

       ///Retrieves instance of current allocator
       /**
        * @return instance of current allocator
        */
       const Alloc &getAllocator() const {
           return *this;
       }

       ///Sets instance of current allocator
       /**
        * @param alloc new instance. It will store copy of the object
        */
       void setAllocator(const Alloc &alloc) {
           Alloc::operator=(alloc);
       }

       ///Copy constructor
       /**
        * Makes copy of target object and allocator
        * @param other source smart pointer.
        *
        * Uses AllocatorBase::copyObj() to perform object copying. Alloc
        * can use this function to implement copying by different way.
        */
       AllocPointer(const AllocPointer &other)
           :Pointer<T>(), Alloc(other.getAllocator()) {
    	   if (other.ptr)
    		   this->ptr = Alloc::createInstance(*other.ptr);
    	   else
    		   other.ptr = 0;
       }

       ///Copy constructor from derived to base instance
       /**
        * Makes copy of target object and allocator
        * @param other source smart pointer.
        *
        * Uses AllocatorBase::copyObj() to perform object copying. Alloc
        * can use this function to implement copying by different way.
        */
       template<typename X>
       AllocPointer(const AllocPointer<X,Alloc> &other)
           :Alloc(other.getAllocator()) {
           this->ptr = this->copyObj(other.ptr);
       }

       ///Assignment operator
       /**
        * Assignment to another pointer causes copying source object into
        * target object. It uses copy constructor (copyObj) not assignment
        * operator. Alloc is also copied using assignment operator
        *
        * @param other source object
        * @return reference to itself
        */
       AllocPointer &operator=(const AllocPointer &other) {
           if (&other != this) {
               clear();
               Alloc::operator=(other.ptr?this->create(*other.ptr):0);
           }
           return *this;
       }

       AllocPointer &operator=(T *other) {
    	   if (this->ptr != other) {
    		   clear();
    		   this->ptr = other;
    	   }
    	   return *this;
       }


       ///Assignment operator
       /**
        * Assignment to another pointer causes copying source object into
        * target object. It uses copy constructor (copyObj) not assignment
        * operator. Alloc is also copied using assignment operator
        *
        * @param other source object
        * @return reference to itself
        */
       template<typename X>
       AllocPointer &operator=(const AllocPointer<X,Alloc> &other) {
           if (&other != *this) {
               clear();
               Alloc::operator=(this->copyObj(other.ptr));

           }
           return *this;
       }


       ///assignes nil to pointer - equivalent to call clear()
       /**
        * @param x nil
        * @return reference to itself
        */
       AllocPointer &operator=(NullType) {
           clear();
           return *this;
       }


       void swap(AllocPointer &other) {
           std::swap(static_cast<Pointer<T> &>(*this),
               static_cast<Pointer<T> &>(other));
           std::swap(static_cast<Alloc &>(*this),
               static_cast<Alloc &>(other));
       }

       AllocPointer(AllocPointer &x, MoveConstruct )
           :Pointer<T>(x.detach()),
            Alloc(_moveConstruct, *this) {
       }

   };

    template<typename A,typename B>
    class MoveObject<AllocPointer<A,B> >: public MoveObject_Construct {};



}

namespace std {
    template<typename A, typename B>
    void swap(::LightSpeed::AllocPointer<A,B> &a, ::LightSpeed::AllocPointer<A,B> &b) {
        a.swap(b);
    }
}


#endif /* _LIGHTSPEED_DYNPOINTER_H_ */
