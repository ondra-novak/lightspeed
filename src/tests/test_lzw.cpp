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
#include "../lightspeed/utils/lzw.h"

namespace LightSpeed {

defineTest test_textToLZWBase64("lzw.compress","UHJpc2VybmUgemx1dG91Y2t5IGt1biB1cGVsIGRhYmVsc2tlIGtvZHk=MTIzNA==",[](PrintTextA &out) {

	ConverterChain<CharsToBytesConvert, ConverterChain<LZWCompress, ByteToBase64Convert>  > convertor;

	ConstStrA text("aaabbbcccabababababacccddeeefeeeeefffeeefefeeeeeaaabbbbeeeefffeefffee");
	              /*aaabbbcccababababbbbcccddeeefeeeeefffeeefefeeeeeaaabbbbeeeefffeeffeee*/
	StringA conv = StringA(convertString(convertor, text));
	out("%1") << conv;
});

defineTest test_LZWBase64ToText("lzw.decompress","Priserne zlutoucky kun upel dabelske kody",[](PrintTextA &out){

	ConverterChain<Base64ToByteConvert, ConverterChain<LZWDecompress, BytesToCharsConvert>  > convertor;

	ConstStrA text("MMCMUEMcGMJihEKhMJgxjMhkMsSM0Sipmi8WMsUiplgUIghijkXjcjiUBA==");
	StringA conv = StringA(convertString(convertor, text));
	out("%1") << conv;

});

}



