/*
 * staticAlloc.h
 *
 *  Created on: 1.1.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MEMORY_STATICALLOC_H_
#define LIGHTSPEED_MEMORY_STATICALLOC_H_

#pragma once

#include <typeinfo>
#include "../exceptions/throws.h"
#include "allocatedMemory.h"

namespace LightSpeed {


	template<natural cnt>
	class StaticAlloc {
	public:


		template<typename T>
		class AllocatedMemory: public ::LightSpeed::AllocatedMemory<T> {
		public:

			T *getBase() const {return reinterpret_cast<T *>(buffer);}
			natural getSize() const {return cnt;}
			bool expand(natural /*items*/) {return false;}
			bool shrink(natural ) {return false;}
			bool swap(AllocatedMemory &) {return false;}
			bool move(AllocatedMemory &) {return false;}
			AllocatedMemory() {}
			AllocatedMemory(const AllocatedMemory &): ::LightSpeed::AllocatedMemory<T>() {}
			AllocatedMemory(const AllocatedMemory &, natural count) {
				if (count > cnt)
					throwAllocatorLimitException(THISLOCATION,count,cnt,typeid(StaticAlloc<cnt>));
			}

			AllocatedMemory(const StaticAlloc<cnt> &, natural count) {
				if (count > cnt)
					throwAllocatorLimitException(THISLOCATION,count,cnt,typeid(StaticAlloc<cnt>));
			}


		protected:
			mutable byte buffer[sizeof(T)*cnt];

		};
		StaticAlloc() {}
		
		
		template<typename T>
		StaticAlloc(const AllocatedMemory<T> &) {}
	};
// 
// 	template<natural cnt>
// 	class StaticAllocString: public StaticAlloc<cnt> {
// 	public:
// 
// 		template<typename T>
// 		class AllocatedMemory: public StaticAlloc<cnt>::template AllocatedMemory<T>
// 								, public ShareableAllocatedMemory 
// 								
// 		{
// 		public:
// 			typedef typename StaticAlloc<cnt>::template AllocatedMemory<T> Super;
// 			T *getBase() const {
// 				if (from) return from->getBase();
// 				return size?reinterpret_cast<T *>(this->buffer):0;
// 			}
// 			natural getSize() const {
// 				if (from) return from->getSize();
// 				return size;
// 			}
// 			bool expand(natural items) {
// 				if (from) return from->expand(items);
// 				if (items > size) {
// 					if (items > cnt) return false;
// 					size = items;
// 				}
// 				return true;					
// 			}
// 			bool shrink(natural items) {
// 				if (from) return from->shrink(items);
// 				if (items < size) 
// 					size = items;
// 				return true;					
// 			}
// 
// 			~AllocatedMemory() {
// 				free();
// 			}
// 			AllocatedMemory():size(0),from(0),to(0) {}
// 			AllocatedMemory(const AllocatedMemory &src):Super(src),size(0),from(0),to(0) {}
// 			AllocatedMemory(const AllocatedMemory &src, natural count)
// 				:Super(src,count),size(count),from(0),to(0) {}
// 
// 			AllocatedMemory(const StaticAlloc<cnt> &alloc, natural count) 
// 				:Super(alloc,count),size(count),from(0),to(0) {}
// 			AllocatedMemory(const AllocatedMemory &src, Share s)
// 				:Super(src,src.size),size(src.size)
// 				,from(const_cast<AllocatedMemory *>(&src))
// 				,to(const_cast<AllocatedMemory *>(src.to))
// 			{				
// /*				for (natural i = 0; i < sizeof(T)*size; i++)
// 					buffer[i] = src.buffer[i];*/
// 				src.to = this;
// 				if (to) to->from = this;
// 			}
// 			AllocatedMemory &operator=(const AllocatedMemory &src) {
// 				if (&src != this) {
// 					free();
// 					from = const_cast<AllocatedMemory *>(&src);
// 					to =const_cast<AllocatedMemory *>(src.to);
// 					src.to = this;
// 					if (to) to->from = this;
// 				}
// 				return *this;
// 			}
// 			bool isShared() const {return to != 0 || from != 0;}
// 			void free() {
// 				if (from) from->to = to;
// 				else if (to) {
// 					for (natural i = 0; i < sizeof(T)*size; i++)
// 						to->buffer[i] = this->buffer[i];
// 					to->size = size;
// 				}
// 				if (to) to->from = from;
// 				size = 0;
// 				to = from = 0;
// 			}
// 		protected:
// 			natural size;
// 			mutable AllocatedMemory *from;
// 			mutable AllocatedMemory *to;
// 		};
// 
// 		StaticAllocString() {}
// 		template<typename T>
// 		StaticAllocString(const AllocatedMemory<T> &e) {}
// 	};
}



#endif /* LIGHTSPEED_MEMORY_STATICALLOC_H_ */
