/*
 * staticAlloc.h
 *
 *  Created on: 1.1.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MEMORY_INBLOCKALLOC_H_
#define LIGHTSPEED_MEMORY_INBLOCKALLOC_H_

#pragma once

#include <typeinfo>
#include "../exceptions/throws.h"
#include "allocatedMemory.h"

namespace LightSpeed {


	class InBlockAlloc {
	public:


		template<typename T>
		class AllocatedMemory: public ::LightSpeed::AllocatedMemory<T> {
		public:

			T *getBase() const {return reinterpret_cast<T *>(addr);}
			natural getSize() const {return cnt/sizeof(T);}
			bool expand(natural /*items*/) {return false;}
			bool shrink(natural ) {return false;}
			bool swap(AllocatedMemory &) {return false;}
			bool move(AllocatedMemory &/*other*/) {return false;}
			AllocatedMemory(const AllocatedMemory &src, natural count) {
				if (count > src.cnt)
					throwAllocatorLimitException(THISLOCATION,count,0,typeid(InBlockAlloc));
				else {
					addr = src.addr;
					cnt = src.cnt;
				}
			}

			AllocatedMemory(const InBlockAlloc &alloc, natural count) {
				if (count > alloc.sz)
					throwAllocatorLimitException(THISLOCATION,count,alloc.sz,typeid(InBlockAlloc));
				addr = alloc.addr;
				cnt = alloc.sz;
			}


		protected:
			void *addr;
			natural cnt;

		};

		InBlockAlloc(void *addr, natural sz):addr(addr),sz(sz) {}
		template<typename T>
		InBlockAlloc(const AllocatedMemory<T> &):addr(0),sz(0) {}
	protected:

		void *addr;
		natural sz;

	};
}


#endif /* LIGHTSPEED_MEMORY_INBLOCKALLOC_H_ */
