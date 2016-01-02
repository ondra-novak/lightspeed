/*
 * stdAlloc.h
 *
 *  Created on: 1.1.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MEMORY_STDALLOC_H_
#define LIGHTSPEED_MEMORY_STDALLOC_H_

#pragma once

#include "allocatedMemory.h"
#include "../exceptions/throws.h"
#include "runtimeAlloc.h"
#include <new>
#include "singleton.h"
#include "../containers/move.h"

namespace LightSpeed {


	class StdAlloc: public IRuntimeAlloc {
	public:

		LIGHTSPEED_CLONEABLECLASS


		template<typename T>
		class AllocatedMemory: public ::LightSpeed::AllocatedMemory<T>, public DetachableAllocatedMemory<T> {
		public:
			typedef ::LightSpeed::AllocatedMemory<T> Super1;
			typedef DetachableAllocatedMemory<T> Super2;
			static const bool isSwapable = true;

			AllocatedMemory():ptr(0),count(0) {}
			AllocatedMemory(natural count)
				:ptr(makeAlloc(count)),count(count) {}
			~AllocatedMemory() {operator delete((void *)ptr);}

			T *getBase() const {return reinterpret_cast<T *>(ptr);}
			natural getSize() const {return count;}
			bool expand(natural ) {return false;}
			bool shrink(natural ) {return false;}
			bool swap(AllocatedMemory &other) {
				T *x = ptr; ptr = other.ptr; other.ptr = x;
				natural c = count;count = other.count; other.count = c;
				return true;
			}
			bool move(AllocatedMemory &other) {return swap(other);}
			void free() {operator delete(ptr);ptr = 0;count = 0;}
			AllocatedMemory(const AllocatedMemory &src)
				:Super1(src),Super2(src),ptr(makeAlloc(src.count)),count(src.count) {}
			AllocatedMemory(const AllocatedMemory &, natural count)
				:ptr(makeAlloc(count)),count(count) {}


			AllocatedMemory(const StdAlloc &, natural count)
				:ptr(makeAlloc(count)),count(count) {}

			T *detach() {
				T *res = reinterpret_cast<T *>(ptr);
				ptr = 0; count = 0;
				return res;
			}

			AllocatedMemory(const StdAlloc &, T *bk, natural sz):ptr(bk),count(sz) {}
			AllocatedMemory(const AllocatedMemory &, T *bk, natural sz):ptr(bk),count(sz) {}

		protected:
			T *ptr;
			natural count;

			friend class StdAlloc;

			static T *makeAlloc(natural count) {
				try {
					if (count) return reinterpret_cast<T *>(operator new(count*sizeof(T)));
					else return 0;
 				} catch (const std::bad_alloc &) {
 					throwOutOfMemoryException(THISLOCATION, count*sizeof(T));
 					return 0;
 				}
			}
		private:
			void operator=(const AllocatedMemory &);
		};

		virtual void *alloc(natural objSize)  {return AllocatedMemory<byte>::makeAlloc(objSize);}
		virtual void *alloc(natural objSize, IRuntimeAlloc *&owner)  {
			owner = this;
			return AllocatedMemory<byte>::makeAlloc(objSize);
		}
		virtual void dealloc(void *ptr, natural /*objSize*/)  {operator delete(ptr);}

		static StdAlloc &getInstance() {
			return Singleton<StdAlloc,true>::getInstance();
		}


		StdAlloc() {}
		template<typename X>
		StdAlloc(const AllocatedMemory<X> &) {}
		StdAlloc(MoveConstruct, StdAlloc &) {}

	};


}


#endif /* LIGHTSPEED_MEMORY_STDALLOC_H_ */
