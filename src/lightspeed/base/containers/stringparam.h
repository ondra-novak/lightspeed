/*
 * cstring.h
 *
 *  Created on: Sep 18, 2012
 *      Author: ondra
 */

#ifndef LIGHTSPEED_STRINGPAR_H_
#define LIGHTSPEED_STRINGPAR_H_
#include "../containers/string.h"


namespace LightSpeed {


///
template<typename T>
class StringParam: public ConstStringT<T> {
public:

	StringParam(const StringCore<T> s):s(s) { ConstStringT<T>::operator=(s);}
	StringParam(const T *str):ConstStringT<T>(str) {}
	StringParam(ConstStringT<T> s):ConstStringT<T>(s) {}

	template<template<class> class Compare>
	StringParam(const StringTC<T,Compare> s):s(s) { ConstStringT<T>::operator=(s);}

	template<template<class> class Compare>
	operator StringTC<T,Compare>() const {
		if (s.empty()) return StringTC<T,Compare>(*this); else return s;
	}

	operator StringCore<T>() const {
		if (s.empty()) return StringCore<T>(*this); else return s;
	}

	operator String() const;

	StringCore<T> getMT() const {
		return s.getMT();
	}

protected:
	StringCore<T> s;

};


template<>
inline StringParam<char>::operator String() const {
	return String(*this);
}

template<>
inline StringParam<wchar_t>::operator String() const {
	if (s.empty()) return String(*this); else return s;
}

typedef StringParam<wchar_t> StrParamW;
typedef StringParam<char> StrParamA;

}


#endif /* LIGHTSPEED_STRINGPAR_H_ */
