/*
 * refCounted.h
 *
 *  Created on: Jan 4, 2016
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_MEMORY_REFCOUNTED_H_
#define LIGHTSPEED_BASE_MEMORY_REFCOUNTED_H_
#include "../../base/sync/threadVar.h"
#include "../../mt/atomic_type.h"
#include "pointer.h"

namespace LightSpeed {


///Faster and still multithread safe implementation of reference-counted objects
class FastMTRefCntObj {
public:
	FastMTRefCntObj() :refCounter(0) {}


	///increases reference
	void addRef() const;
	///decrements reference
	/** @note even with zero reference, object will not destroyed until its pointer leaves local
	 *  pointer map. If you need to ensure, that object will be removed with its last reference
	 *  you have to call commitRef() before last releaseRef(). However, when there is another thread
	 *  keeping its reference, commitRef() will not work
	 */
	void releaseRef() const;
	///Removes reference from the local pointer map commiting current counter to the global counter
	/** This can help to release object sooner when function is called before last reference
	 * is removed. When releaseRef() is called on object with one reference, pointer is not stored in
	 * the pointer map, but it is destroyed directly.
	 */
	void commitRef() const;

	///Removes all references from the local pointer map.
	static void commitAllRefs();

	///disables auto-release on the object.
	/** If you allocated object statically, you don't want to call delete on that object, however you
	 * still need to pass object as FastMTRefCntPtr which tracks its reference. Call this fuction
	 * to make this happen. Object will receive highest counter value with impossible to reduce this
	 * counter to zero.
	 */
	void setStaticObj();

	///All object must have virtual destructors to handle correctly deletion
	virtual ~FastMTRefCntObj() {}
protected:
	mutable atomic refCounter;
	static const atomicValue threadRef = atomicValue(1) << (sizeof(atomicValue)*5);


	class  PointerCache {
	public:
		static const natural hashTableSize = 16;


		PointerCache() {
			for (natural i = 0; i < hashTableSize; i++) {
				pointers[i] = 0;
			}
		}
		~PointerCache();


		const FastMTRefCntObj *pointers[hashTableSize];
		atomicValue counts[hashTableSize];


		static natural hashPointer(const FastMTRefCntObj *ptr);
		integer findItem(const FastMTRefCntObj *ptr);
		void flushCacheItem(natural index);
		void flushAll();
	};

	static ThreadVarInitDefault<PointerCache> cachePtr;

	static atomicValue lockAdd(atomic &v, atomicValue val);
	static atomicValue lockXAdd(atomic &v, atomicValue val);

};

template<typename T>
class FastMTRefCntPtr: public Pointer<T> {
public:

	FastMTRefCntPtr() {}
	FastMTRefCntPtr(T *ptr):Pointer<T>(ptr) {
		if (ptr) ptr->addRef();
	}
	FastMTRefCntPtr(const FastMTRefCntPtr &other):Pointer<T>(other.ptr) {
		if (other.ptr) other.ptr->addRef();
	}
	FastMTRefCntPtr(const Pointer<T> &other):Pointer<T>(other.ptr) {
		if (other.ptr) other.ptr->addRef();
	}

	FastMTRefCntPtr &operator=(const FastMTRefCntPtr &other) {
		if (other.ptr != this->ptr) {
			other.ptr->addRef();
			try {
				this->ptr->release();
				this->ptr = other.ptr;
			} catch (...) {
				other.ptr->release();
				throw;
			}
		}
		return *this;
	}

};


inline FastMTRefCntObj::PointerCache::~PointerCache()
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
			TLSTable &tls;
			AllocPointer<PointerCache> tmp;
			Deleter(PointerCache *owner):owner(owner),pos(0)
				,tls(TLSTable::getInstance())
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


inline void FastMTRefCntObj::PointerCache::flushAll() {
	natural pos = 0;
	//runs till end reached
	while (pos < hashTableSize) {
		//increase pos
		natural i = pos++;
		//if pointer exist
		if (pointers[i])
			//flush it
			flushCacheItem(i);
	}

}

inline natural FastMTRefCntObj::PointerCache::hashPointer(const FastMTRefCntObj* ptr) {
	natural v = (natural)ptr;
	const natural maskShift = sizeof(v) * 8 - 1; 
	if (sizeof(v) >= 8) v = v ^ (v >> (32 & maskShift));
	if (sizeof(v) >= 4) v = v ^ (v >> (16 & maskShift));
	if (sizeof(v) >= 2) v = v ^ (v >> (8 & maskShift));
	v = v ^ (v >> 4);
	return v & 0xF;

}

///find cache item for given pointer
/**
 * @param pointer
 * @retval >=0 found and function returns index
 * @retval <0 not found. Add hashTableSize to get index
 */
inline integer FastMTRefCntObj::PointerCache::findItem(const FastMTRefCntObj *ptr) {
	natural pos = hashPointer(ptr);
	if (pointers[pos] == ptr) {return pos;}
	else return integer(pos) - hashTableSize;
}



inline void FastMTRefCntObj::PointerCache::flushCacheItem(natural pos) {
	//read pointer
	const FastMTRefCntObj *ptr = pointers[pos];
	//set item to zero
	pointers[pos] = 0;
	//update objects counter
	atomicValue x = lockAdd(ptr->refCounter, counts[pos] - FastMTRefCntObj::threadRef);
	//if counter is zero, delete the object (can throw an exception)
	if (x == 0) delete ptr;
}


inline void FastMTRefCntObj::addRef() const {
	//find pointer cache
	PointerCache &cache = cachePtr[TLSTable::getInstance()];
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
		lockXAdd(refCounter,threadRef);
		cache.pointers[p] = this;
		cache.counts[p] = 1;
	} else {
		natural p(r);
		++cache.counts[p];
	}
}


inline void FastMTRefCntObj::releaseRef() const {
	PointerCache &cache = cachePtr[TLSTable::getInstance()];
	integer r = cache.findItem(this);
	if (r < 0) {

		if (lockXAdd(refCounter,threadRef) == 1) {
			lockXAdd(refCounter,-threadRef-1);
			delete this;
			return;
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
		--cache.counts[p];
	}
}

inline void FastMTRefCntObj::commitRef() const {
	PointerCache &cache = cachePtr[TLSTable::getInstance()];
	integer r = cache.findItem(this);
	if (r >= 0) {
		cache.flushCacheItem(r);
	}
}


} /* namespace LightSpeed */

#endif /* LIGHTSPEED_BASE_MEMORY_REFCOUNTED_H_ */
