/*
 * newline.cpp
 *
 *  Created on: 13.3.2011
 *      Author: ondra
 */

#include "../text/newline.h"

namespace LightSpeed {

DefaultNLString<char>::DefaultNLString():ConstStringT<char>("\r\n") {}

DefaultNLString<wchar_t>::DefaultNLString():ConstStringT<wchar_t>(L"\r\n") {}

}
