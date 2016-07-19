/*
 * test_base64.cpp
 *
 *  Created on: 17. 7. 2016
 *      Author: ondra
 */
#include "../lightspeed/base/containers/constStr.h"
#include "../lightspeed/base/framework/testapp.h"
#include "../lightspeed/base/containers/convertString.h"
#include "../lightspeed/utils/json/jsonserializer.tcc"
#include "../lightspeed/base/streams/utf.tcc"

namespace LightSpeed {

defineTest test_texttoUtf8("utf8.textToUtf8","Příšerně žluťoučký kůň úpěl ďábelské kódy",[](PrintTextA &out) {

	WideToUtf8Convert convertor;

	ConstStrW text(L"Příšerně žluťoučký kůň úpěl ďábelské kódy");
	StringA conv = StringA(convertString(convertor, text));
	out("%1") << conv;

});

defineTest test_utf8ToTest("utf8.textToWide","Příšerně žluťoučký kůň úpěl ďábelské kódy",[](PrintTextA &out) {

	Utf8ToWideConvert convertor;

	ConstStrA text("Příšerně žluťoučký kůň úpěl ďábelské kódy");
	String conv = String(convertString(convertor, text));
	out("%1") << conv.getUtf8();

});
}



