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
#include "../lightspeed/utils/base16.h"

namespace LightSpeed {

defineTest test_textToBase16("baseX.textToBase16","5072697365726E65207A6C75746F75636B79206B756E207570656C20646162656C736B65206B6F647931323334",[](PrintTextA &out) {

	CharsToBytesConvert charToByte;
	ByteToBase16Convert base16conv;
	ConverterChain<CharsToBytesConvert &, ByteToBase16Convert &> convChain(charToByte, base16conv);

	ConstStrA text("Priserne zlutoucky kun upel dabelske kody");
	StringA conv = StringA(convertString(convChain, text));
	out("%1") << conv;

	base16conv.eolb = false;

	ConstStrA text2("1234");
	StringA conv2 = StringA(convertString(convChain, text2));
	out("%1") << conv2;
});

defineTest test_base16ToText("baseX.base16ToText","Priserne zlutoucky kun upel dabelske kody",[](PrintTextA &out){


	BytesToCharsConvert charToByte;
	Base16ToByteConvert base16conv;
	ConverterChain<Base16ToByteConvert &, BytesToCharsConvert &> convChain(base16conv,charToByte);

	ConstStrA text("5072697365726E65207A6C75746F75636B79206B756E207570656C20646162656C736B65206B6F6479");
	StringA conv = StringA(convertString(convChain, text));
	out("%1") << conv;

});

}



