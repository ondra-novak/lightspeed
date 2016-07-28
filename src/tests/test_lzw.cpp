/*
 * test_base64.cpp
 *
 *  Created on: 17. 7. 2016
 *      Author: ondra
 */
#include "../lightspeed/base/containers/constStr.h"
#include "../lightspeed/base/framework/testapp.h"
#include "../lightspeed/base/containers/convertString.h"
#include "../lightspeed/base/exceptions/errorMessageException.h"
#include "../lightspeed/base/streams/standardIO.h"
#include "../lightspeed/utils/json/jsonserializer.tcc"
#include "../lightspeed/utils/base64.h"
#include "../lightspeed/utils/lzw.h"

extern const char *test_data_json;
extern unsigned int test_data_json_length;


namespace LightSpeed {

defineTest test_textToLZWBase64("lzw.compress","MMCMUEMcGMJihEKhMJgxjMhkMsSM0Sipmi8WMsUiplgUIghijkXjcjiUBA==",[](PrintTextA &out) {

	ConverterChain<CharsToBytesConvert, ConverterChain<LZWCompress, ByteToBase64Convert>  > convertor;

	ConstStrA text("aaabbbcccabababababacccddeeefeeeeefffeeefefeeeeeaaabbbbeeeefffeefffee");
	              /*aaabbbcccabababababacccddeeefeeeeefffeeefefeeeeeaaabbbbeeeefffeefffee*/
	StringA conv = StringA(convertString(convertor, text));
	out("%1") << conv;
});

defineTest test_LZWBase64ToText("lzw.decompress","aaabbbcccabababababacccddeeefeeeeefffeeefefeeeeeaaabbbbeeeefffeefffee",[](PrintTextA &out){

	ConverterChain<Base64ToByteConvert, ConverterChain<LZWDecompress, BytesToCharsConvert>  > convertor;
				  /*MMCMUEMcGMJihEKhMJgxjMhkMsSM0Sipmi8WMsUiplgUIghijkXjcjiUBA==*/
	ConstStrA text("MMCMUEMcGMJihEKhMJgxjMhkMsSM0Sipmi8WMsUiplgUIghijkXjcjiUBA==");
	StringA conv = StringA(convertString(convertor, text));
	out("%1") << conv;

});


defineTest test_lzwLargeFile("lzw.largeFile","34113",[](PrintTextA &out) {

	ConstBin srcFile(test_data_json, test_data_json_length);

	StringB conv = StringB(convertString(LZWCompress(12), srcFile));
	StringB deconv = StringB(convertString(LZWDecompress(), ConstBin(conv)));

	out("%1") << (deconv == srcFile?conv.length():0);
});

defineTest test_lzwpLargeFile("lzw.lzwp-largeFile","28929",[](PrintTextA &out) {

	ConstBin srcFile(test_data_json, test_data_json_length);

 	StringB conv = StringB(convertString(LZWpCompress(12), srcFile));
	StringB deconv = StringB(convertString(LZWpDecompress(), ConstBin(conv)));

	out("%1") << (deconv == srcFile?conv.length():0);
});

defineTest test_lzwpZeroTest("lzw.lzwp-zerotest","7854",[](PrintTextA &out) {

	natural count=10*1024*1024;
	AutoArrayStream<byte> compressed;
	{
		ConvertWriteIter<LZWpCompress, AutoArrayStream<byte> &> compress(LZWpCompress(12),compressed);

		for (natural i = 0; i < count; i++)
			compress.write(0);

		compress.flush();
	}
	{
		ConstBin data(compressed.getArray());
		ConvertReadIter<LZWpDecompress, ConstBin::Iterator> decompress(data.getFwIter());

		natural c = 0;
		while (decompress.hasItems()) {
			byte b = decompress.getNext();
			if (b != 0) throw ErrorMessageException(THISLOCATION,"Unexpected byte (must be 0)");
			c++;
		}
		if (c < count)
			throw ErrorMessageException(THISLOCATION,"Read bytes less than expected ");
		if (c > count)
			throw ErrorMessageException(THISLOCATION,"Read bytes more than expected ");
	}
	out("%1") << compressed.length();
});


}



