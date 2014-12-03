/*
 * cloneable.h
 *
 *  Created on: 12.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MEMORY_CLONEABLE_H_
#define LIGHTSPEED_MEMORY_CLONEABLE_H_

#include "../meta/isConvertible.h"
#include "../meta/isDynamic.h"
#include "dynobject.h"


namespace LightSpeed {

class IRuntimeAlloc;

class StdAlloc;


	class ICloneable {
	public:

		virtual ~ICloneable() {}

		///Retrieves size of object need for allocation using the clone
		/**
		 * @return size in bytes. Note that returned value is slightly 
		 * larger than real size of object, because function calculates extra
		 * bytes need to store pointer to allocator (IRuntimeAlloc)
		 */
		virtual natural getObjectSize() const = 0;
		///Clones object using specified allocator
		/**
		 * @param alloc allocator used to allocate space. 
		 * @return pointer to copy of object. Specified allocator must remain valid
		 * during whole lifetime of the object. Result pointer can be released
		 * by standard delete operator. It will still use allocator to free
		 * space
		 */
		 
		virtual ICloneable *clone(IRuntimeAlloc &alloc) const = 0;
		///Creates copy in space allocated by standard allocator
		/** result object is smaller, because in this case, there is not need
		* to store pointer to allocator
		*/
		virtual ICloneable *clone() const = 0;



		template<typename T>
		class ClonedObject: public T, public DynObject  {
		public:
			ClonedObject(const T &x):T(x) {}
		};


		template<typename T>
		static T *doCloneObject(const T *orig,  IRuntimeAlloc &alloc) {
			return doCloneObject(orig, alloc, typename MIsConvertible<T *, DynObject *>::MValue());
		}
		template<typename T>
		static T *doCloneObject(const T *orig) {
			return new T(*orig);
		}

		template<typename T>
		static natural getObjectSize(const T *x) {
			return getObjectSize(x, typename MIsConvertible<T *, DynObject *>::MValue());
		}

		typedef ICloneable CloneableBase;
private:
		template<typename T>
		static T *doCloneObject(const T *orig,  IRuntimeAlloc &alloc, MFalse) {
			return new(DynObjectAllocHelper(alloc)) ClonedObject<T>(*orig);
		}

		template<typename T>
		static natural getObjectSize(const T *, MFalse) {
			return DynObject::SizeOfObject<ClonedObject<T> >::size;
		}

		template<typename T>
		static T *doCloneObject(const T *orig,  IRuntimeAlloc &alloc, MTrue) {
			return new(DynObjectAllocHelper(alloc)) T(*orig);
		}

		template<typename T>
		static natural getObjectSize(const T *, MTrue) {
			return DynObject::SizeOfObject<T>::size;
		}
	};


#define LIGHTSPEED_CLONEABLEDECL(ClassName) \
		typedef ClassName ICloneableBase; \
		virtual ICloneableBase *clone(::LightSpeed::IRuntimeAlloc &alloc) const = 0; \
		virtual ICloneableBase *clone() const = 0;

#define LIGHTSPEED_CLONEABLECLASS \
	virtual ICloneableBase *clone(::LightSpeed::IRuntimeAlloc &alloc) const {\
		return ::LightSpeed::ICloneable::doCloneObject(this,alloc); \
	} \
	virtual ICloneableBase *clone() const { \
		return ::LightSpeed::ICloneable::doCloneObject(this); \
	} \
	virtual ::LightSpeed::natural getObjectSize() const {return ::LightSpeed::ICloneable::getObjectSize(this);}

#define LIGHTSPEED_NOT_CLONEABLECLASS \
	virtual ICloneableBase *clone(::LightSpeed::IRuntimeAlloc &) const {\
		throwUnsupportedFeature(THISLOCATION,this,"clone - class inherits ICloneable but disabled clone");return 0; \
	} \
	virtual ICloneableBase *clone() const { \
		throwUnsupportedFeature(THISLOCATION,this,"clone - class inherits ICloneable but disabled clone");return 0; \
	} \
	virtual ::LightSpeed::natural getObjectSize() const {return ::LightSpeed::ICloneable::getObjectSize(this);}



}  // namespace LightSpeed

#endif /* LIGHTSPEED_MEMORY_CLONEABLE_H_ */
