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
#include "../lightspeed/utils/base32.h"

namespace LightSpeed {

defineTest test_textToBase32("baseX.textToBase32","KBZGS43FOJXGKID2NR2XI33VMNVXSIDLOVXCA5LQMVWCAZDBMJSWY43LMUQGW33EPE======GEZDGNA=",[](PrintTextA &out) {

	CharsToBytesConvert charToByte;
	ByteToBase32Convert base32conv;
	ConverterChain<CharsToBytesConvert &, ByteToBase32Convert &> convChain(charToByte, base32conv);

	ConstStrA text("Priserne zlutoucky kun upel dabelske kody");
	StringA conv = StringA(convertString(convChain, text));
	out("%1") << conv;

	base32conv.eolb = false;

	ConstStrA text2("1234");
	StringA conv2 = StringA(convertString(convChain, text2));
	out("%1") << conv2;
});

defineTest test_base32ToText("baseX.base32ToText","Priserne zlutoucky kun upel dabelske kody",[](PrintTextA &out){


	BytesToCharsConvert charToByte;
	Base32ToByteConvert base32conv;
	ConverterChain<Base32ToByteConvert &, BytesToCharsConvert &> convChain(base32conv,charToByte);

	ConstStrA text("KBZGS43FOJXGKID2NR2XI33VMNVXSIDLOVXCA5LQMVWCAZDBMJSWY43LMUQGW33EPE======");
	StringA conv = StringA(convertString(convChain, text));
	out("%1") << conv;

});

}



