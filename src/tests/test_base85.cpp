
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
#include "../lightspeed/utils/base85.h"

namespace LightSpeed {

defineTest test_textToBase85("baseX.textToBase85","p&Zn>wPzW1aBm<DBz(qiyK44rB-5sAAaJ&dwmn}Uy&:x6azU[aC(",[](PrintTextA &out) {

	CharsToBytesConvert charToByte;
	ByteToBase85Convert base85conv;
	ConverterChain<CharsToBytesConvert &, ByteToBase85Convert &> convChain(charToByte, base85conv);

	ConstStrA text("Priserne zlutoucky kun upel dabelske kody");
	StringA conv = StringA(convertString(convChain, text));
	out("%1") << conv;

});

defineTest test_base85ToText("baseX.base85ToText","Priserne zlutoucky kun upel dabelske kody",[](PrintTextA &out){


	BytesToCharsConvert charToByte;
	Base85ToByteConvert base85conv;
	ConverterChain<Base85ToByteConvert &, BytesToCharsConvert &> convChain(base85conv,charToByte);

	ConstStrA text("p&Zn>wPzW1aBm<DBz(qiyK44rB-5sAAaJ&dwmn}Uy&:x6azU[aC(");
	StringA conv = StringA(convertString(convChain, text));
	out("%1") << conv;

});

}



