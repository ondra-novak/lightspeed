#pragma once

#ifndef _LIGHTSPEED_BASE_SYNC_LOCKPTR_BREDY_
#define _LIGHTSPEED_BASE_SYNC_LOCKPTR_BREDY_
#include "../../mt/fastlock.h"
#include "../memory/pointer.h"
#include "../sync/synchronize.h"


namespace LightSpeed {

///Use LockPtr to hold pointer and lock on single smart pointer
/**
 * @tparam T type of smart pointer
 * @tparam Lock type of lock
 * @tparam Base implementation of pointer, default is Pointer<T>
 */
template<typename T, typename Lock = FastLockR, typename Base = Pointer<T> >
class LockPtr: public Base, public Synchronized<Lock> {
public:

	LockPtr(T *object, Lock &lock):Base(object),Synchronized<Lock>(lock) {}
	LockPtr(const LockPtr &other):Base(other),Synchronized<Lock>(other) {}
	LockPtr(const Base &base, Lock &lock):Base(base),Synchronized<Lock>(lock) {}



};




}


#endif
