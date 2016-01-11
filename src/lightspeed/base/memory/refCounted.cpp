/*
 * refCounted.cpp
 *
 *  Created on: Jan 4, 2016
 *      Author: ondra
 */

#include "refCounted.h"
#include "../../base/sync/threadVar.h"
#include "../../mt/atomic.h"
namespace LightSpeed {

static inline atomicValue lockAdd(atomic &v, atomicValue val) {
	return lockExchangeAdd(v, val) + val;
}

const atomicValue RefCounted::threadRef = atomicValue(1) << (sizeof(atomicValue)*5);

class  PointerCache {
public:
	static const natural hashTableSize = 16;


	PointerCache() {
		for (natural i = 0; i < hashTableSize; i++) {
			pointers[i] = 0;
		}
	}
	~PointerCache();


	const RefCounted *pointers[hashTableSize];
	atomicValue counts[hashTableSize];


	static natural hashPointer(const RefCounted *ptr);
	integer findItem(const RefCounted *ptr);
	void flushCacheItem(natural index);
};

static ThreadVarInitDefault<PointerCache> cachePtr;


PointerCache::~PointerCache()
{
	//so during destruction, there will be a lot object releases
	//these releases can decrement reference counters in this thread
	//so we will need to prepare temporary pointer cache for that

	//first check whether this is necessary for prevent infinite loop
	bool needrun = false;
	for (natural i = 0; i < hashTableSize && !needrun; i++) {
		//found pointer, we will run cleanup
		if (pointers[i]) needrun = true;
	}

	//check whether we need to run
	if (needrun) {

		//cleans table of pointers
		//if item throws an exception, deleter will continue to destroy other objects
		//but uncaught exception will be emmited so other objects should not throw exception
		//otherwise double exception error can happen
		class Deleter {
		public:
			PointerCache *owner;
			natural pos;
			//create temporary pointer cache
			//set temporary cache to tls index
			//at this point, index should be already empty
			//this prevents to recreation table during mass destruction
			ITLSTable &tls;
			AllocPointer<PointerCache> tmp;
			Deleter(PointerCache *owner):owner(owner),pos(0)
				,tls(ITLSTable::getInstance())
				,tmp(new PointerCache){

				cachePtr.ThreadVar::set(tls,tmp.get());
			}
			void run() {
				//runs till end reached
				while (pos < hashTableSize) {
					//increase pos
					natural i = pos++;
					//if pointer exist
					if (owner->pointers[i])
						//flush it
						owner->flushCacheItem(i);
				}
			}
			//continue to run if necessary
			~Deleter() {
				run();
				//unset the table index
				//now destructor of temporary table can destroy all next entries
				//destructor of tmp table will continue in cleanup
				cachePtr.ThreadVar::set(tls,0);
			}
		};

		//clear whole table
		Deleter(this).run();

	}
}

/* 00,11,22,33,44,55,66,77,88,99,AA,BB,CC,DD,EE,FF + 17
 *
 */

inline natural PointerCache::hashPointer(const RefCounted* ptr) {
	natural v = (natural)ptr;
	v = v ^ (v >> 32);
	v = v ^ (v >> 16);
	v = v ^ (v >> 8);
	v = v ^ (v >> 4);
	return v & 0xF;

}

///find cache item for given pointer
/**
 * @param pointer
 * @retval >=0 found and function returns index
 * @retval <0 not found. Add hashTableSize to get index
 */
inline integer PointerCache::findItem(const RefCounted *ptr) {
	natural pos = hashPointer(ptr);
	if (pointers[pos] == ptr) {return pos;}
	else return integer(pos) - hashTableSize;
}


RefCounted::RefCounted():refCounter(0) {

}

void PointerCache::flushCacheItem(natural pos) {
	//read pointer
	const RefCounted *ptr = pointers[pos];
	//set item to zero
	pointers[pos] = 0;
	//update objects counter
	atomicValue x = lockAdd(ptr->refCounter, counts[pos] - RefCounted::threadRef);
	//if counter is zero, delete the object (can throw an exception)
	if (x == 0) delete ptr;
}


void RefCounted::addRef() const {
	//find pointer cache
	PointerCache &cache = cachePtr[ITLSTable::getInstance()];
	//find pointer in cache
	integer r = cache.findItem(this);
	//if not found (r ~ <-hashTableSIze,-1>
	if (r < 0) {
		//calculate suitable index for storing the pointer
		natural p(r+PointerCache::hashTableSize);
		//if index is occupied, remove item first
		/*we have to repeat this because object can be destroyed and destructur
		 *  can release references, which may cause that place will be re-occupied again */
		while (cache.pointers[p]) cache.flushCacheItem(p);
		lockExchangeAdd(refCounter,threadRef);
		cache.pointers[p] = this;
		cache.counts[p] = 0;
	} else {
		natural p(r);
		cache.counts[p]++;
	}
}


void RefCounted::release() const {
	PointerCache &cache = cachePtr[ITLSTable::getInstance()];
	integer r = cache.findItem(this);
	if (r < 0) {

		if (lockExchangeAdd(refCounter,threadRef) == 1) {
			lockExchangeSub(refCounter,threadRef+1);
			delete this;
		}

		//calculate suitable index for storing the pointer
		natural p(r+PointerCache::hashTableSize);
		//if index is occupied, remove item first
		/*we have to repeat this because object can be destroyed and destructur
		 *  can release references, which may cause that place will be re-occupied again */
		while (cache.pointers[p])
			cache.flushCacheItem(p);
		cache.pointers[p] = this;
		cache.counts[p] = -1;
		return;
	} else {
		natural p(r);
		cache.counts[p]++;
	}
}



} /* namespace LightSpeed */


