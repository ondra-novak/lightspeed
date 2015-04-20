/*
 * nameToEnum.cpp
 *
 *  Created on: 25.3.2014
 *      Author: ondra
 */

#include "namedEnum.h"

#include "../base/containers/autoArray.tcc"
namespace LightSpeed {

ConstStrA UnknownEnumName::getName() const {
	return name;
}

ConstStrA UnknownEnumName::getSet() const {
	return set;
}

void UnknownEnumName::message(ExceptionMsg& msg) const {
	msg("Unknown enum value: %1. Expected one of: %2") << name << set;
}

template class AutoArray<char, SmallAlloc<256> >;

} /* namespace coinstock */

