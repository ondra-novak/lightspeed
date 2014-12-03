/*
 * newline.h
 *
 *  Created on: 13.3.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_TEXT_NEWLINE_H_
#define LIGHTSPEED_TEXT_NEWLINE_H_

#include "../containers/constStr.h"

namespace LightSpeed {

template<typename T>
class DefaultNLString: public ConstStringT<T> {
public:
	DefaultNLString():ConstStringT<T>() {}
};


template<>
class DefaultNLString<char>: public ConstStringT<char> {
public:
	DefaultNLString();
};

template<>
class DefaultNLString<wchar_t>: public ConstStringT<wchar_t> {
public:
	DefaultNLString();
};



}

#endif /* LIGHTSPEED_TEXT_NEWLINE_H_ */
