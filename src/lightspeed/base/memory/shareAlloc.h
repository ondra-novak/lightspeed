/*
 * shareAlloc.h
 *
 *  Created on: 1.1.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MEMORY_SHAREALLOC_H_
#define LIGHTSPEED_MEMORY_SHAREALLOC_H_


#include "stdAlloc.h"
#include "refCntPtr.h"

namespace LightSpeed {

#ifdef _WIN32
#pragma warning (disable : 4522)
#endif


class ShareAlloc {


	template<typename T>
	class Header: public RefCntObj {
	public:


		struct AllocInfo {
			 IRuntimeAlloc *allocator;
			std::size_t size;
		};

		natural getCount() const {
			const AllocInfo *ai = reinterpret_cast<const AllocInfo *>(this);
			--ai;
			return (ai->size - sizeof(AllocInfo) - sizeof(Header)) / sizeof(T);
		}

		IRuntimeAlloc *getAllocator() const {
			const AllocInfo *ai = reinterpret_cast<const AllocInfo *>(this);
			--ai;
			return ai->allocator;
		}

		void *operator new(std::size_t objsize,  IRuntimeAlloc &alloc, natural size) {

			std::size_t allsize = sizeof(AllocInfo)  + size + objsize;
			IRuntimeAlloc *newa;
			void *p = alloc.alloc(allsize,newa);
			AllocInfo *ap = reinterpret_cast<AllocInfo *>(p);
			ap->allocator = newa;
			ap->size = allsize;
			return ap+1;
		}

		void operator delete(void *ptr,  IRuntimeAlloc &, natural ) {
			destroyInstance(ptr);
		}

		void operator delete(void *ptr) {
			destroyInstance(ptr);
		}

		static void destroyInstance(void *ptr) {
			AllocInfo *ap = reinterpret_cast<AllocInfo *>(ptr);
			--ap;
			ap->allocator->dealloc(ap,ap->size);
		}
	};

	template<typename T>
	friend class StdFactory::Factory;

	template<typename T>
	class HeaderDebug: public Header<T> {
	public:
		T data[100];
	};


public:



	template<typename T>
	class AllocatedMemory: public ::LightSpeed::AllocatedMemory<T>,
	public ShareableAllocatedMemory {
	public:

		AllocatedMemory():ptr(0) {}
		AllocatedMemory(const ShareAlloc &master,natural count)
			:ptr(static_cast<HeaderDebug<T> *>(new(master.rtAlloc,count * sizeof(T)) Header<T>())) {}

		T *getBase() const {return ptr == nil?0:reinterpret_cast<T *>(ptr->data);}
		natural getSize() const {return ptr == nil?0:ptr->getCount();}
		bool expand(natural ) {return false;}
		bool shrink(natural ) {return false;}
		bool swap(AllocatedMemory &other) {
			std::swap(ptr,other.ptr);
			return true;
		}
		bool move(AllocatedMemory &other) {return swap(other);}
		void free() {ptr = nil;}
		AllocatedMemory(const AllocatedMemory &src)
			:ptr(src.ptr == nil?0:
				static_cast<HeaderDebug<T> *>(new(*src.ptr->getAllocator(),src.getSize() * sizeof(T)) Header<T>())) {}

		AllocatedMemory(const AllocatedMemory &src, Share /*s*/)
			:ptr(src.ptr) {}

		template<typename X>
		AllocatedMemory(const typename ShareAlloc::template AllocatedMemory<T> &src, Share)
			:ptr(src.ptr) {}

		AllocatedMemory(const AllocatedMemory &src, natural count)
			:ptr(count?static_cast<HeaderDebug<T> *>(new(src.ptr != nil?*src.ptr->getAllocator():StdAlloc::getInstance(),count * sizeof(T)) Header<T>()):0) {}

		AllocatedMemory(const ShareAlloc &a)
			:ptr(static_cast<HeaderDebug<T> *>(new(a.rtAlloc,0) Header<T>())) {}



		AllocatedMemory &operator=(const AllocatedMemory &other) {
			ptr = other.ptr;
			return *this;
		}

		template<typename X>
		AllocatedMemory &operator=(const typename ShareAlloc::template AllocatedMemory<T> &other) {
			ptr = other.ptr;
			return *this;
		}

		bool isShared() const {return ptr.isShared();}


	protected:
		RefCntPtr<HeaderDebug<T>,StdFactory::Factory<Header<T> > > ptr;
		friend class ShareAlloc;

	};

	ShareAlloc():rtAlloc(StdAlloc::getInstance()) {}
	ShareAlloc( IRuntimeAlloc &a):rtAlloc(a) {}

	template<typename X>
	explicit ShareAlloc(const AllocatedMemory<X> &x):rtAlloc(x.ptr == nil?StdAlloc::getInstance():*x.ptr->getAllocator()) {}
protected:
	 IRuntimeAlloc &rtAlloc;
};


}  // namespace LightSpeed

#endif /* LIGHTSPEED_MEMORY_SHAREALLOC_H_ */

