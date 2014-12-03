/*
 * mtrefcntptr.h
 *
 *  Created on: 10.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_MTREFCNTPTR_H_
#define LIGHTSPEED_MT_MTREFCNTPTR_H_

#pragma once

#include "../base/memory/refCntPtr.h"
#include "../base/memory/stdFactory.h"

namespace LightSpeed {

template<typename T, typename Alloc =  StdFactory::Factory<T> >
class MTRefCntPtr: public RefCntPtr<T,Alloc> {
public:
	typedef RefCntPtr<T,Alloc> Super;

	MTRefCntPtr() {}
	MTRefCntPtr(NullType x):Super(x) {}
	MTRefCntPtr(T *p):Super(p) {this->ptr->enableMTAccess();}
    MTRefCntPtr(NullType x, const Alloc &r):Super(x,r) {}
    MTRefCntPtr(T *ptr,const Alloc &r):Super(ptr,r) {this->ptr->enableMTAccess();}
    MTRefCntPtr(const MTRefCntPtr &other):Super(other) {}
    MTRefCntPtr(const RefCntPtr<T,Alloc> &other):Super(other) {this->ptr->enableMTAccess();}
    operator MTRefCntPtr<const T,Alloc>() const {
    	return RefCntPtr<const T,Alloc>(this->ptr,*this);
    }
    MTRefCntPtr &operator=(const MTRefCntPtr &other) {
    	Super::operator =(other);
    	return *this;
    }

    MTRefCntPtr &operator=(T *ptr) {
    	Super::operator =(ptr);
    	return *this;
    }

};

}


#endif /* LIGHTSPEED_MT_MTREFCNTPTR_H_ */
