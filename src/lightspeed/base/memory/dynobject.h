#pragma once

#include <cstddef>
#include "../types.h"

namespace LightSpeed {

	class IRuntimeAlloc;

	///Stored extra informations to recover when allocation is dropped by the exception.
	/**  Because there is deficiency in C++ standard when size_t of the
	     allocated object is not reported to the operator delete in case
		 of exception during construction, instance of this class is
		 used to store size during allocation and construction.

		 Instance is destroyed, when object is constructed, or when
		 exception is thrown.

		 During allocation, operator new stores size of the allocated 
		 object into this object. When exception is caught, stored size
		 is used to proper call of dealloc function.
	*/


	class DynObjectAllocHelper {
	public:

		///implicit constructor - creates instance implicitly
		DynObjectAllocHelper (IRuntimeAlloc &alloc):alloc(alloc),sz(0) {}
		IRuntimeAlloc & ref() const {return alloc;}

		///fuction is const to able carry instance as const X &
		/** Can change variable, because it is declared as mutable */
		void storeSize(std::size_t sz)  const {this->sz = sz;}
		std::size_t getSize() const {return sz;}


	protected:
		IRuntimeAlloc & alloc;
		mutable std::size_t sz;
	};


	///Useful as root class for all object which can be allocated by extended new operator
	/**
	 * All objects, which inherits this object will able to
	 * be created using extended new operator which supports various
	 * allocators
	 *
	 * @note You can choose allocator as advantage. Deletion of such
	 * object doesn't need to special action, because standard delete
	 * operator is also overloaded and will use same allocator to free
	 * the allocated memory. This needs to keep allocator's instance
	 * during whole lifetime of this object (allocators handle self 
	 * destruction automatically, when last object is released from its
	 * pool). Disadvantage is that allocation takes extra four bytes 
	 * (eight bytes on 64 bit platform) to store pointer to allocator. This
	 * also includes allocation using standard new, when extra bytes are
	 * zeroed to identify the standard new allocation.
	 *
	 * Array allocations are not handled
	 *
	 * @note DON'T FORGET VIRTUAL DESTRUCTOR ON POLYMORPHICS OBJECTS
	 * 
	 */
	 
	class DynObject {
	public:
		
		///object can be allocated using standard new	
		void *operator new(std::size_t sz);
		///object can be also allocated using allocator carried trough DynObjectAllocHelper 
		void *operator new(std::size_t sz, const DynObjectAllocHelper &alloc);	
		///placement allocator - you can call delete on such object 
		void *operator new(std::size_t, void *ptr) {;return ptr;}

		///Standard delete - called when delete operator used
		/**
		 * @param ptr pointer to object
		 * @param sz size of object calculated by the compiler 
		 */		 
		void operator delete(void *ptr, std::size_t sz);

		///Called when exception thrown during standard new
		/** Size is not carried, but standard allocator can handle this */
//		void operator delete(void *ptr);
		///Called when exception thrown during allocation using custom allocator		
		void operator delete(void *ptr, const DynObjectAllocHelper &alloc);
		///Called when exception thrown during placement allocator
		void operator delete(void *, void *) {}


		template<typename T> struct SizeOfObject {
			static const natural size = ((sizeof(T) + sizeof(void *) - 1) & ~(sizeof(void *) - 1)) + sizeof (IRuntimeAlloc *);
		};
	private:
		///error of usage - did you forget to add * before variable?
		void *operator new(std::size_t, IRuntimeAlloc *ptr);
		///error of usage - did you forget to add * before variable?
		void *operator new(std::size_t, const IRuntimeAlloc *ptr);
		///error of usage - did you forget to add * before variable?
		void operator delete(void *ptr, IRuntimeAlloc *alloc);
		///error of usage - did you forget to add * before variable?
		void operator delete(void *ptr, const IRuntimeAlloc *alloc);

	};

	template<typename T>
	class MakeDynObject: public T, public DynObject {
	public:
		MakeDynObject() {}
		MakeDynObject(const T &other):T(other) {}
	};

	template<natural sz>
	class AllocAtStack;

	template<typename T, natural count=1>
	class AllocAtStackObj: public AllocAtStack<DynObject::template SizeOfObject<T>::size * count > {};
	

}
