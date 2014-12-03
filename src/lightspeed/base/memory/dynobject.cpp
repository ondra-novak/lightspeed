#include "dynobject.h"
#include "runtimeAlloc.h"


namespace LightSpeed {

static inline std::size_t alignSize(std::size_t sz) {
	//HACK Align is always power of 2 - we can use & ~ to replace division and multiplication
	return (sz + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
}

static inline std::size_t expandSize(std::size_t sz) {
	return alignSize(sz) + sizeof(IRuntimeAlloc *);
}

static inline IRuntimeAlloc **getAllocStorage(void *ptr, std::size_t sz) {
	return reinterpret_cast<IRuntimeAlloc **>(reinterpret_cast<byte *>(ptr) + alignSize(sz));
}

/*
static inline const IRuntimeAlloc *const *getAllocStorage(const void *ptr, std::size_t sz) {
	return reinterpret_cast<const IRuntimeAlloc *const *>(reinterpret_cast<const byte *>(ptr) + alignSize(sz));
}*/

void * DynObject::operator new( std::size_t sz )
{
	void *res = ::operator new(expandSize(sz));
	IRuntimeAlloc **store = getAllocStorage(res,sz);
	*store = 0;
	return res;
}

void * DynObject::operator new( std::size_t sz, const DynObjectAllocHelper &alloc )
{
	IRuntimeAlloc *owner = 0;
	void *res = alloc.ref().alloc(expandSize(sz),owner);
	*getAllocStorage(res,sz) = owner;
	alloc.storeSize(sz);
	return res;
}

void  DynObject::operator delete( void *ptr, const DynObjectAllocHelper &alloc )
{
	IRuntimeAlloc *a = *getAllocStorage(ptr,alloc.getSize());
	a->dealloc(ptr,expandSize(alloc.getSize()));

}

void  DynObject::operator delete( void *ptr, std::size_t sz )
{
	IRuntimeAlloc *alloc = *getAllocStorage(ptr,sz);
	if (alloc == 0)
		::operator delete(ptr);
	else
		alloc->dealloc(ptr,expandSize(sz));
}

}
