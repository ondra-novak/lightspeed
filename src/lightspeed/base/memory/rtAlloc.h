/*
 * cloneAlloc.h
 *
 *  Created on: 2.1.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MEMORY_CLONEALLOC_H_
#define LIGHTSPEED_MEMORY_CLONEALLOC_H_


#include "allocatedMemory.h"
#include "../exceptions/throws.h"
#include "runtimeAlloc.h"
#include "factory.h"
#include <new>
#include "stdFactory.h"

namespace LightSpeed {

	///RTAlloc implements AllocatedMemory class set using IRuntimeAlloc
	/** This is similar allocator as StdAlloc, but you can specify allocator
	 * which implements IRuntimeAlloc interface
	 *
	 * RTAlloc object is not larger than StdAlloc, because it also occupies one
	 * DWORD (or QWORD) in memory. AllocatedMemory stores pointer to allocator 
	 * in allocated block, and uses allocator instance hint to fast deallocation
	 */
	class RTAlloc {
	public:
		
		template<typename T>
		class AllocatedMemory: public ::LightSpeed::AllocatedMemory<T>, public DetachableAllocatedMemory<T> {
		public:
			AllocatedMemory(IRuntimeAlloc &rt) {
				allocBlock(rt,0);
			}
			AllocatedMemory(IRuntimeAlloc &rt, natural size) {
				allocBlock(rt,size);
			}

			AllocatedMemory(const RTAlloc &rt) {
				allocBlock(rt.rt,0);
			}
			AllocatedMemory(const RTAlloc &rt, natural size) {
				allocBlock(rt.rt,size);
			}

			AllocatedMemory(const AllocatedMemory &a) {
				allocBlock(*a.getAllocator(),a.size);
			}

			AllocatedMemory(const AllocatedMemory &a, natural size) {
				allocBlock(*a.getAllocator(),size);
			}

			~AllocatedMemory() {
				if (size) (*ptr)->dealloc(ptr,calcSpace(size));
			}

	
			T *getBase() const {return reinterpret_cast<T *>(ptr+1);}

			natural getSize() const {return size;}

			bool expand(natural ) {return false;}
			bool shrink(natural ) {return false;}
			bool move(AllocatedMemory &other) {return swap(other);}
			bool swap(AllocatedMemory &other) {
				std::swap(ptr,other.ptr);
				std::swap(size,other.size);
				return true;
			}

			static natural calcSpace(natural size) {
				return size*sizeof(T) + sizeof(IRuntimeAlloc *);
			}

			IRuntimeAlloc *getAllocator() const {
				if (size) return *ptr; else return alloc;
			}


		protected:
			union {
				///when length contains a nonzero, there is pointer to the allocated block
				/** In allocated block, pointer to IRuntimeAlloc is first */
				IRuntimeAlloc **ptr;
				///when length contains the zero, pointer points directly to an instance of IRuntimeAlloc
				IRuntimeAlloc *alloc;
			};
			natural size;

			void allocBlock(IRuntimeAlloc &rt, natural items) {
				if (items == 0) {
					alloc = &rt;
					size = 0;
				} else {
					natural sz = calcSpace(items);
					IRuntimeAlloc *out;
					void *p = rt.alloc(sz,out);
					ptr = reinterpret_cast<IRuntimeAlloc **>(p);
					*ptr = out;
					size = items;
				}

			}

		
		};

		RTAlloc(IRuntimeAlloc &rt):rt(rt) {}
		RTAlloc():rt(StdFactory::getInstance()) {}

		template<typename T>
		RTAlloc(const AllocatedMemory<T> &block) 
			:rt(block.getAllocator()) {}
	
		
		IRuntimeAlloc *getAllocator() const {			
			return &rt;
		}

	protected:
		IRuntimeAlloc &rt;


	};

	///Implements Factory using IRuntimeAlloc
	class RTFactory: public RTAlloc {
	public:

		template<typename T>
		class Factory: public FactoryBase<T,Factory<T> > {
		public:

			void *alloc(natural sz) {
				return a.alloc(sz);
			}

			void dealloc(void *ptr, natural sz) {
				return a.dealloc(ptr,sz);
			}

			void dealloc(void *ptr) {
				return a.dealloc(ptr,sizeof(T));
			}

			Factory(IRuntimeAlloc &a):a(a) {}
			Factory(const RTFactory &other):a(other.rt) {}

			template<typename X>
			Factory(const Factory<X> &other):a(other.a) {}

		protected:
			IRuntimeAlloc &a;
		};


		RTFactory(IRuntimeAlloc &rt):RTAlloc(rt) {}

	};

#if 0


	class RTAlloc {
	public:


		template<typename T>
		class AllocatedMemory: public ::LightSpeed::AllocatedMemory<T>, public DetachableAllocatedMemory<T> {
		public:


			AllocatedMemory(IRuntimeAlloc &rt):rt(rt),ptr(0),count(0) {}
			~AllocatedMemory() {if (ptr) rt.dealloc(ptr,count*sizeof(T));}

			T *getBase() const {return reinterpret_cast<T *>(ptr);}
			natural getSize() const {return count;}
			bool expand(natural items) {return false;}
			bool shrink(natural items) {return false;}
			bool swap(AllocatedMemory &other) {
				void *x = ptr; ptr = other.ptr; other.ptr = x;
				natural c = count;count = other.count; other.count = c;
				return true;
			}
			bool move(AllocatedMemory &other) {return swap(other);}
 			AllocatedMemory(const AllocatedMemory &src)
				:rt(src.rt),ptr(makeAlloc(src.rt,src.count)),count(src.count) {}
			AllocatedMemory(const AllocatedMemory &src, natural count)
				:rt(src.rt),ptr(makeAlloc(src.rt,count)),count(count) {}


			AllocatedMemory(const RTAlloc &alloc, natural count)
				:rt(alloc.rt),ptr(makeAlloc(alloc.rt,count)),count(count) {}

			T *detach() {
				T *res = reinterpret_cast<T *>(ptr);
				ptr = 0; count = 0;
				return res;
			}

			AllocatedMemory(const RTAlloc &alloc, T *bk, natural sz):rt(alloc.rt),ptr(bk),count(sz) {}
			AllocatedMemory(const AllocatedMemory &alloc, T *bk, natural sz):rt(alloc.rt),ptr(bk),count(sz) {}

			IRuntimeAlloc &getRTAlloc() const {return rt;}

		protected:
			IRuntimeAlloc &rt;
			void *ptr;
			natural count;

			static void *makeAlloc(IRuntimeAlloc &rt,natural count) {
				if (count) return rt.alloc(count*sizeof(T));
				else return 0;
			}
		private:
			void operator=(const AllocatedMemory &);
		};

		RTAlloc(IRuntimeAlloc &rt):rt(rt) {}
		template<typename T>
		RTAlloc(const AllocatedMemory<T> &other):rt(other.getRTAlloc()) {}

		IRuntimeAlloc &getRTAlloc() {return rt;}

	protected:
		IRuntimeAlloc &rt;
	};

	class RTFactory: public RTAlloc, public IRuntimeAlloc {
	public:

		template<typename T>
		class Factory: public FactoryBase<T,Factory<T> > {
		public:

			void *alloc(natural sz) {
				return a.alloc(sz);
			}

			void dealloc(void *ptr, natural sz) {
				return a.dealloc(ptr,sz);
			}

			void dealloc(void *ptr) {
				return a.dealloc(ptr,sizeof(T));
			}

			Factory(IRuntimeAlloc &a):a(a) {}
			Factory(const RTFactory &other):a(other.rt) {}

			template<typename X>
			Factory(const Factory<X> &other):a(other.a) {}

		protected:
			IRuntimeAlloc &a;
		};


		RTFactory(IRuntimeAlloc &rt):RTAlloc(rt) {}

		virtual void *alloc(natural objSize) {return rt.alloc(objSize);}
		virtual void *alloc(natural objSize, IRuntimeAlloc * &owner) {return rt.alloc(objSize,owner);}
		virtual void dealloc(void *ptr, natural objSize) {return rt.dealloc(ptr,objSize);}
		virtual AllocLevel getAllocLevel() const {return rt.getAllocLevel();}


	};
#endif
}


#endif /* LIGHTSPEED_MEMORY_CLONEALLOC_H_ */
