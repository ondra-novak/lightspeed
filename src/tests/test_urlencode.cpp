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
#include "../lightspeed/utils/urlencode.h"

namespace LightSpeed {

defineTest test_urlEncode_encode("urlEncode.encode","%7B%22text%22%3A%22P%C5%99%C3%AD%C5%A1ern%C4%9B%20%C5%BElu%C5%A5ou%C4%8Dk%C3%BD%20k%C5%AF%C5%88%20%C3%BAp%C4%9Bl%20%C4%8F%C3%A1belsk%C3%A9%20k%C3%B3dy%22%7D",[](PrintTextA &out) {

	UrlEncodeConvert convertor;

	ConstStrA text("{\"text\":\"Příšerně žluťoučký kůň úpěl ďábelské kódy\"}");
	StringA conv = StringA(convertString(convertor, text));
	out("%1") << conv;

});

defineTest test_urlEncode_decode("urlEncode.decode","{\"text\":\"Příšerně žluťoučký kůň úpěl ďábelské kódy\"}",[](PrintTextA &out) {

	UrlDecodeConvert convertor;

	ConstStrA text("%7B%22text%22%3A%22P%C5%99%C3%AD%C5%A1ern%C4%9B+%C5%BElu%C5%A5ou%C4%8Dk%C3%BD%20k%C5%AF%C5%88%20%C3%BAp%C4%9Bl%20%C4%8F%C3%A1belsk%C3%A9%20k%C3%B3dy%22%7D");
	StringA conv = StringA(convertString(convertor, text));
	out("%1") << conv;

});
}



