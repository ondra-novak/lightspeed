/*
 * variant.cpp
 *
 *  Created on: Jul 20, 2016
 *      Author: ondra
 */

#include "variant.h"

namespace LightSpeed {


Variant::Variant() {}

const std::type_info& Variant::getType() const {
	if (dataPtr == null) throwNullPointerException(THISLOCATION);
	return dataPtr->getType();
}

bool Variant::isNull() const {
	return dataPtr == null;
}

Variant Variant::isolate() const {
	if (dataPtr == null) return Variant();
	else return Variant(dataPtr->clone());
}

Variant Variant::getMT() const {
	return Variant(dataPtr.getMT());
}

bool Variant::operator ==(NullType ) const {
	return isNull();
}

bool Variant::operator !=(NullType ) const {
	return !isNull();
}

bool Variant::operator ==(const Variant& v) const {
	if (v.isNull()) return isNull();
	if (isNull()) return v.isNull();
	return dataPtr->same(v.dataPtr);
}

bool Variant::operator !=(const Variant& v) const {
	return !operator==(v);
}

}
