
/*
 * smallAlloc.h
 *
 *  Created on: 2.1.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MEMORY_SMALLALLOC_H_
#define LIGHTSPEED_MEMORY_SMALLALLOC_H_


#pragma once

#include <typeinfo>
#include "../exceptions/throws.h"
#include "allocatedMemory.h"

namespace LightSpeed {


	///Handles small array allocations faster using preallocated space
	/**
	 * SmallAlloc is designed to provide allocations in AutoArray and similar object
	 * faster, when expected count of items is small. You have to specify
	 * expected count of items in template argument.
	 *
	 * Allocator reserves buffer for the items inside of instance of itself, so
	 * there is no need to allocate memory at heap. If AutoArray object is located
	 * at the stack, preallocated space is also created at the stack.
	 *
	 * This allocator is similar to StaticAlloc, but allows allocation beyond specified
	 * limit, when allocations are made at the heap
	 *
	 * @tparam cnt reserved count of items
	 **/
	template<natural cnt>
	class SmallAlloc {
	public:

		IRuntimeAlloc *alloc;


		SmallAlloc(IRuntimeAlloc &alloc):alloc(&alloc) {}
		SmallAlloc(IRuntimeAlloc *alloc):alloc(alloc) {}
		SmallAlloc():alloc(0) {}


		template<typename T>
		class AllocatedMemory: public ::LightSpeed::AllocatedMemory<T> {
		public:

			bool swap(AllocatedMemory &other) {
				if (isLocal() || other.isLocal()) return false;
				std::swap(ptr, other.ptr);
				std::swap(size, other.size);
				std::swap(alloc, other.alloc);
				return true;
			}

			bool move(AllocatedMemory &other) {
				if (isLocal()) return false;
				if (!other.isLocal()) {
					operator delete(other.ptr);
				}
				other.ptr = ptr;
				other.size = size;
				other.alloc = alloc;
				ptr = 0;
				size = 0;
				alloc = 0;
				return true;

			}

			AllocatedMemory(const AllocatedMemory &src, natural count): alloc(src.alloc) {
				allocMem(count);
			}



			AllocatedMemory(const SmallAlloc<cnt> &alloc, natural count):alloc(alloc.alloc) {
				allocMem(count);
			}

			AllocatedMemory(const SmallAlloc<cnt> &alloc):alloc(alloc.alloc) {

			}


			AllocatedMemory(const AllocatedMemory &src):alloc(src.alloc) {
				allocMem(src.size);
			}

			~AllocatedMemory() {
				if (!isLocal())  {
					if (alloc) alloc->dealloc(ptr,size*sizeof(T));
					else operator delete(ptr);
				}			
			}

			bool expand(natural) {return false;}
			bool shrink(natural ) {return false;}

			T *getBase() const {
				T *x = reinterpret_cast<T *>(ptr);
				return x;
			}
			natural getSize() const {
				return size;
			}


			IRuntimeAlloc *getRtAlloc() const {return alloc;}
		protected:
			IRuntimeAlloc *alloc;
			void *ptr;
			natural size;
			mutable byte buffer[cnt * sizeof(T)];

			bool isLocal() const {return buffer == reinterpret_cast<const byte *>(ptr);}

			void allocMem(natural count) {
				if (count > cnt) {
					try {
						if (alloc) ptr = alloc->alloc(sizeof(T) * count);
						else ptr = operator new(sizeof(T) * count);
					} catch (const std::bad_alloc &) {
	 					throwOutOfMemoryException(THISLOCATION, count*sizeof(T));
					}
					size = count;
				} else {
					ptr = buffer;
					size = cnt;
				}
			}
		};

		template<typename T>
		SmallAlloc(const AllocatedMemory<T> &a):alloc(a.getRtAlloc()) {}

	};
}



#endif /* LIGHTSPEED_MEMORY_SMALLALLOC_H_ */
