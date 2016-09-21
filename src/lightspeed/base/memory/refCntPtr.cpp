#include "refCntPtr.h"
#include "../../mt/atomic.h"

namespace LightSpeed {






void RefCntObj::debugClear() const {
	writeRelease(&counter,0xFEEEFEEE);
}





void LightSpeed::RefCntObj::globalDisableMTAccess() {
}

void LightSpeed::RefCntObj::globalEnableMTAccess() {
}

///Sets counter to ST

}
