#include "refCntPtr.h"
#include "../../mt/atomic.h"

namespace LightSpeed {




void RefCntObj::addRefMT() const {
	atomic *i = reinterpret_cast<atomic *>(&counter);
	lockInc(*i);
}

bool RefCntObj::releaseMT() const {
	atomic *i = reinterpret_cast<atomic *>(&counter);
	return lockDec(*i) == 0;
}

void RefCntObj::addRef() const {
#ifdef _DEBUG
			if (readAcquire(&counter) ==  0xFEEEFEEE) debugBreak();
#endif
			if ((counter & fastFlag)) {
				++counter;
			}else{
				addRefMT();
			}
}

bool RefCntObj::release() const {
#ifdef _DEBUG
			if (readAcquire(&counter) ==  0xFEEEFEEE) debugBreak();
#endif
			if ((counter & fastFlag)) {
				return --counter == fastFlag;
			} else {
				return releaseMT();
			}
}

void RefCntObj::debugClear() const {
	writeRelease(&counter,0xFEEEFEEE);
}

void RefCntObj::enableMTAccess() const {
	atomicValue v = readAcquire(&counter);
	atomicValue nv,ov;
	do {
		if ((v & fastFlag) == 0) break;
		ov = v;
		nv = ov & ~fastFlag;
	} while ((v = lockCompareExchange(counter,ov,nv)) != ov);
}

void RefCntObj::disableMTAccess() const {
	atomicValue v = readAcquire(&counter);
	atomicValue nv,ov;
	do {
		if ((v & fastFlag) != 0) break;
		ov = v;
		nv = ov | fastFlag;
	} while ((v = lockCompareExchange(counter,ov,nv)) != ov);
}


atomicValue RefCntObj::initialValue = RefCntObj::fastFlag;


void LightSpeed::RefCntObj::globalDisableMTAccess() {
	initialValue = fastFlag;
}

void LightSpeed::RefCntObj::globalEnableMTAccess() {
	initialValue = 0;
}

///Sets counter to ST

}
