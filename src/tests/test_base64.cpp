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
#include "../lightspeed/utils/base64.h"

namespace LightSpeed {

defineTest test_textToBase64("baseX.textToBase64","UHJpc2VybmUgemx1dG91Y2t5IGt1biB1cGVsIGRhYmVsc2tlIGtvZHk=MTIzNA==",[](PrintTextA &out) {

	CharsToBytesConvert charToByte;
	ByteToBase64Convert base64conv;
	ConverterChain<CharsToBytesConvert &, ByteToBase64Convert &> convChain(charToByte, base64conv);

	ConstStrA text("Priserne zlutoucky kun upel dabelske kody");
	StringA conv = StringA(convertString(convChain, text));
	out("%1") << conv;

	base64conv.eolb = false;

	ConstStrA text2("1234");
	StringA conv2 = StringA(convertString(convChain, text2));
	out("%1") << conv2;
});

defineTest test_base64ToText("baseX.base64ToText","Priserne zlutoucky kun upel dabelske kody",[](PrintTextA &out){


	BytesToCharsConvert charToByte;
	Base64ToByteConvert base64conv;
	ConverterChain<Base64ToByteConvert &, BytesToCharsConvert &> convChain(base64conv,charToByte);

	ConstStrA text("UHJpc2VybmUgemx1dG91Y2t5IGt1biB1cGVsIGRhYmVsc2tlIGtvZHk=");
	StringA conv = StringA(convertString(convChain, text));
	out("%1") << conv;

});

}



