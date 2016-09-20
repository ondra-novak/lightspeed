/*
 * refCounted.cpp
 *
 *  Created on: 20. 9. 2016
 *      Author: ondra
 */

#include <functional>
#include "refCounted.h"

#include "../../mt/atomic.h"
#include "../../mt/threadHook.h"

namespace LightSpeed {



RefCounted::RefCounted():counter(0) {}



RefCounted::~RefCounted() {}


class RefCounted::PointerCacheItem {
public:

	PointerCacheItem():pointer(0),counter(0) {}

	bool flush() {
		if (counter) {
			//there can happen a collision when item is reused during the flush
			//so we need to re-check, that counter is really zero after the flush
			//if not, repeat the flush operation, because somebody really
			//need this item empty
			do {
				//save values first - it can be rewrited during this operation
				atomicValue z = counter;
				const RefCounted *p = pointer;
				//reset counter - slot is no longer used
				counter = 0;
				//update global counter
				p->updateCounterMt(z);
				//done for this item
			} while (counter);
			//report that some flush has been performed
			return true;
		} else {
			//no action taken
			return false;
		}
	}
	void updateCounter(const RefCounted *newp, atomicValue diff) {
		//pointers differ?
		if (pointer != newp) {
			//flush item
			flush();
			//store item
			pointer = newp;
			//increase one reference for the thread which holds the reference
			pointer->updateCounterMt(1);
			//however, remove this reference from the local counter
			counter = diff-1;
		} else {
			counter += diff;
		}
	}

	const RefCounted * getPtr() const {return pointer;}

	atomicValue getCounterDiff(const RefCounted *ptr) const {
		if (pointer == ptr) return counter;
		else return 0;
	}

protected:
	const RefCounted *pointer;
	atomicValue counter;

};

class RefCounted::PointerCache {
public:

	///size of pointer cache
	/** the value must be powered by 2^n (32,64,128,256,...) */
	static const natural cacheSize = 64;


	void updateCounter(const RefCounted *ptr, atomicValue diff);
	void flush(const RefCounted *ptr);
	PointerCache ();
	~PointerCache();

	static thread_local RefCounted::PointerCache pointerCache;

	void flushTable() throw();
	natural getCounter(const RefCounted *ptr) const;

protected:

	PointerCacheItem table[cacheSize];

};

class CleanupThreadHook: public AbstractThreadHook {
	virtual void onThreadExit(Thread &)  {
		RefCounted::commitAllRefs();
	}
	virtual void onThreadException(Thread &) throw() {
		RefCounted::commitAllRefs();
	}

};

static CleanupThreadHook hk;

RefCounted::PointerCache::PointerCache () {
	hk.install();
}

thread_local RefCounted::PointerCache RefCounted::PointerCache::pointerCache;


static inline natural hashPointer(const RefCounted *ptr) {

	Bin::natural32 x;
	if (sizeof(ptr) > 4) {
		Bin::natural32 *a = reinterpret_cast<Bin::natural32 *>(&ptr);
		x = a[0] ^ a[1];
	} else {
		x = *reinterpret_cast<Bin::natural32 *>(&ptr);
	}
	Bin::natural16 x2;
	if (sizeof(ptr) > 2) {
		Bin::natural16 *a = reinterpret_cast<Bin::natural16 *>(&x);
		x2 = a[0] ^ a[1];
	} else {
		x2 = Bin::natural16(x);
	}
	Bin::natural8 x3;
	{
			Bin::natural8 *a = reinterpret_cast<Bin::natural8 *>(&x2);
			x3 = a[0] ^ a[1];
	}
	return x3;
}


void RefCounted::PointerCache::updateCounter(const RefCounted *ptr, atomicValue diff) {
	natural index = hashPointer(ptr) & (cacheSize-1);
	PointerCacheItem &citem = table[index];
	citem.updateCounter(ptr,diff);
}

void RefCounted::PointerCache::flush(const RefCounted* ptr) {
	natural index = hashPointer(ptr) & (cacheSize-1);
	PointerCacheItem &citem = table[index];
	if (citem.getPtr() == ptr) citem.flush();
}

RefCounted::PointerCache::~PointerCache() {
	flushTable();
}
void RefCounted::PointerCache::flushTable() throw() {
	//during destruction (performed by stdlib at the end of the thread)
	//we need to flush all records in the table and update object countes
	//this may cause, that some counters reach the zero. If this
	//happen, some new records may reappear, as the objects are leaving
	//references they keeping
	//The destructor must repeatedly clean the table until there
	//is no items to flush

	bool rep = true;
	while (rep) {
		rep = false;
		for (natural i = 0;i < cacheSize; i++) {
			try {
				if (table[i].flush()) rep = true;
			} catch (...) {

			}
		}
	}

}

void RefCounted::updateCounter(atomicValue diff) const {
	PointerCache::pointerCache.updateCounter(this,diff);
}

bool RefCounted::isShared() const {
	atomicValue cnt = counter + PointerCache::pointerCache.getCounter(this);
	return cnt > 1;
}

void LightSpeed::RefCounted::commitAllRefs() throw() {
	PointerCache::pointerCache.flushTable();
}


void RefCounted::commitRef() const {
	PointerCache::pointerCache.flush(this);

}


void RefCounted::updateCounterMt(atomicValue diff) const {
	atomicValue x = lockExchangeAdd(counter, diff);
	if (x + diff == 0)
		delete this;
}

natural RefCounted::PointerCache::getCounter(const RefCounted* ptr) const {
	natural index = hashPointer(ptr) & (cacheSize-1);
	const PointerCacheItem &citem = table[index];
	return citem.getCounterDiff(ptr);

}

}

