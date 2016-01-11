/*
 * refCounted.cpp
 *
 *  Created on: Jan 4, 2016
 *      Author: ondra
 */

#include "fastMtRefCnt.h"
#include "../../base/sync/threadVar.h"
#include "../../mt/atomic.h"
namespace LightSpeed {


atomicValue FastMTRefCntObj::lockAdd(atomic &v, atomicValue val) {
	return lockExchangeAdd(v, val) + val;
}

void FastMTRefCntObj::commitAllRefs() {
	PointerCache &cache = cachePtr[ITLSTable::getInstance()];
	cache.flushAll();
}

void FastMTRefCntObj::setStaticObj() {
	refCounter = atomicValue(1) << (sizeof(atomicValue)*8-1);
}

atomicValue FastMTRefCntObj::lockXAdd(atomic& v, atomicValue val) {
	return lockExchangeAdd(v, val);
}

ThreadVarInitDefault<FastMTRefCntObj::PointerCache> FastMTRefCntObj::cachePtr;


} /* namespace LightSpeed */


