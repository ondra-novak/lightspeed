#ifndef LIGHTSPEED_META_METAIF_H_
#define LIGHTSPEED_META_METAIF_H_

#include "metaConst.h"

namespace LightSpeed {

	///Derives T1 or T2 depends on boolean condition
	template<bool cond, class T1, class T2> class MIf;
	template<class T1, class T2> class MIf<false,T1,T2>: public T2 {public: typedef T2 T;};
	template<class T1, class T2> class MIf<true,T1,T2>: public T1 {public: typedef T1 T;};

	
	template<class T1,class T2> class MIsSame: public MConst<bool, false> {};
	template<class T> class MIsSame<T,T>: public MConst<bool, true> {};
	
	template<class T1> class MIsPointer: public MConst<bool, false> {};
	template<class T1> class MIsPointer<T1 *>: public MConst<bool, true> {};
	template<class T1> class MIsPointer<const T1 *>: public MConst<bool, true> {};


}

#endif /*METAIF_H_*/
