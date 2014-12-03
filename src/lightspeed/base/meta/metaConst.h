#ifndef LIGHTSPEED_META_METACONST_H_
#define LIGHTSPEED_META_METACONST_H_

#include "truefalse.h"

namespace LightSpeed {

	template<class T, T val>
	struct MConst {
		static const T value = val;
	};

	template<> struct MConst<bool, true>: MTrue {};
	template<> struct MConst<bool, false>: MFalse {};
	
}


#endif /*METACONST_H_*/
