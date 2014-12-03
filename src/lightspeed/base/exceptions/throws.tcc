/*
 * throws.tcc
 *
 *  Created on: 6.10.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_EXCEPTIONS_THROWS_TCC_
#define LIGHTSPEED_EXCEPTIONS_THROWS_TCC_

#include "unsupportedFeature.h"
#include "rangeException.h"
#include "throws.h"

namespace LightSpeed {

template<typename X>
inline void throwUnsupportedFeature(const ProgramLocation &loc,
                        const X *,
                        const char *feature) {

    throw UnsupportedFeatureOnClass<X>(feature,loc);
}


template<typename T>
inline void throwRangeException_FromTo(const ProgramLocation &loc,
            T rangeFrom , T rangeTo, T val) {
    throw RangeException<T>(loc,RangeExceptMode::rangeFromTo,
            rangeFrom,rangeTo,val);
}
template<typename T>
inline void throwRangeException_From(const ProgramLocation &loc,
            T rangeFrom , T val) {
    throw RangeException<T>(loc,RangeExceptMode::rangeFrom,
            rangeFrom,val);
}
template<typename T>
inline void throwRangeException_To(const ProgramLocation &loc,
            T rangeTo , T val) {
    throw RangeException<T>(loc,RangeExceptMode::rangeTo,
            rangeTo,val);
}
template<typename T>
inline void throwRangeException_NoRange(const ProgramLocation &loc,T val) {
    throw RangeException<T>(loc,RangeExceptMode::noRange,
            val);
}

template<typename T>
inline void throwRangeException(const ProgramLocation &loc) {
    throw RangeException<T>(loc);
}


}  // namespace LightSpeed

#endif /* LIGHTSPEED_EXCEPTIONS_THROWS_TCC_ */
