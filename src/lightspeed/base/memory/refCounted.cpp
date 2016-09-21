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
#include "../../base/platform.h"

namespace LightSpeed {



RefCounted::RefCounted():counter(0) {}



RefCounted::~RefCounted() {}


class RefCounted::PointerCacheItem {
public:

	PointerCacheItem():pointer(0),counter(0) {}

	bool flush() {
		if (likely(counter != 0)) {
			//there can happen a collision when item is reused during the flush
			//so we need to re-check, that counter is really zero after the flush
			//if not, repeat the flush operation, because somebody really
			//need this item empty
			do {
				//save values first - it can be rewritten during this operation
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
	template<atomicValue diff>
	void updateCounter(const RefCounted *newp);

	const RefCounted * getPtr() const {return pointer;}

	atomicValue getCounterDiff(const RefCounted *ptr) const {
		if (pointer == ptr) return counter;
		else return 0;
	}

	const RefCounted *pointer;
	atomicValue counter;

};

class RefCounted::PointerCache {
public:

	///size of pointer cache
	/** the value must be powered by 2^n (32,64,128,256,...) */
	static const natural cacheSize = 64;


	template<atomicValue diff>
	void updateCounter(const RefCounted *ptr);
	void flush(const RefCounted *ptr);
	PointerCache ();
	~PointerCache();

	static thread_local RefCounted::PointerCache *pointerCache;


	void flushTable() throw();
	natural getCounter(const RefCounted *ptr) const;

protected:

	PointerCacheItem table[cacheSize];

};

class RefCounted::PointerDmz: public RefCounted::PointerCache {
public:
	static RefCounted dmzItem;
	static RefCounted::PointerDmz dmzTable;


	PointerDmz() {
		for (natural i = 0; i < cacheSize; i++) {
			table[i].pointer = &dmzItem;
			table[i].counter = 12345678;
		}
	}

};

class CleanupThreadHook: public AbstractThreadHook {
public:
	CleanupThreadHook() {
		RefCounted::onThreadInit();
	}
	~CleanupThreadHook() {
		RefCounted::onThreadExit();
	}

	virtual void onThreadInit(Thread &) {
		RefCounted::onThreadInit();
	}

	virtual void onThreadExit(Thread &)  {
		RefCounted::onThreadExit();
	}
	virtual void onThreadException(Thread &) throw() {
		RefCounted::onThreadExit();
	}

};
static CleanupThreadHook hk;

//fake global table which is initialized by fake item (dmzItem)
//In case that thread-local table is not initialized, or already destroyed, this causes that
//incrementing and decrementing is directly routed to the interlocked function
//Using the table means, that there is no extra instruction in time-critical part of reference counting
//however, it wastes 512 bytes of global heap

RefCounted::PointerDmz RefCounted::PointerDmz::dmzTable;
RefCounted RefCounted::PointerDmz::dmzItem;
thread_local RefCounted::PointerCache *RefCounted::PointerCache::pointerCache = &RefCounted::PointerDmz::dmzTable;



template<atomicValue diff>
inline void RefCounted::PointerCacheItem::updateCounter(const RefCounted *newp) {
	//pointers differ?
	if (likely(pointer == newp)) {
		counter += diff;
	} else {
		//in this case, test whether we are working with dmzTable
		if (unlikely(pointer == &RefCounted::PointerDmz::dmzItem)) {
			//if we are, route directly to interlocked
			newp->updateCounterMt(diff);
		} else {
			//flush item
			flush();
			//increase one reference for the thread which holds the reference
			atomicValue r = newp->updateCounterMt(1);
			//in case that previous count + diff == 0
			if (unlikely((r + diff) == 0)) {
				//this was last reference, so we can destroy object now
				delete newp;
			} else {
				//store item
				pointer = newp;
				//however, remove this reference from the local counter
				counter = diff-1;
			}
		}
	}
}


void RefCounted::onThreadExit() {
	if (RefCounted::PointerCache::pointerCache != &RefCounted::PointerDmz::dmzTable) {
		//store pointer
		RefCounted::PointerCache *c = RefCounted::PointerCache::pointerCache;
		//make table destroyed (all other request will be routed to the dmzTable
		RefCounted::PointerCache::pointerCache = &RefCounted::PointerDmz::dmzTable;
		//delete table (will be flushed)
		delete c;
	}
}

void RefCounted::onThreadInit() {
	if (RefCounted::PointerCache::pointerCache != &RefCounted::PointerDmz::dmzTable) return;
	RefCounted::PointerCache::pointerCache = new PointerCache;
}


RefCounted::PointerCache::PointerCache () {
	hk.install();
}



static inline natural hashPointer(const RefCounted *ptr) {


	natural b = reinterpret_cast<natural>(&ptr);
	return b ^ (b >> 8) ^ (b >> 16) ^ (b >> 24);

}

template<atomicValue diff>
inline void RefCounted::PointerCache::updateCounter(const RefCounted *ptr) {
	natural index = hashPointer(ptr) & (cacheSize-1);
	PointerCacheItem &citem = table[index];
	citem.updateCounter<diff>(ptr);
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

template<atomicValue diff>
void RefCounted::updateCounter() const {
	PointerCache::pointerCache->updateCounter<diff>(this);
}

bool RefCounted::isShared() const {
	atomicValue cnt = counter + PointerCache::pointerCache->getCounter(this);
	return cnt > 1;
}

void LightSpeed::RefCounted::commitAllRefs() throw() {
	PointerCache::pointerCache->flushTable();
}


void RefCounted::commitRef() const {
	PointerCache::pointerCache->flush(this);

}


atomicValue RefCounted::updateCounterMt(atomicValue diff) const {
	atomicValue x = lockExchangeAdd(counter, diff);
	if (x + diff == 0)
		delete this;
	return x;
}

natural RefCounted::PointerCache::getCounter(const RefCounted* ptr) const {
	natural index = hashPointer(ptr) & (cacheSize-1);
	const PointerCacheItem &citem = table[index];
	return citem.getCounterDiff(ptr);

}

template void RefCounted::updateCounter<1>() const;
template void RefCounted::updateCounter<-1>() const;
}

