
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
#include "../lightspeed/base/iter/placeholders.tcc"

namespace LightSpeed {

defineTest test_placeholders("Convertor.placeholders","Happy birthday dear Jessie, happy birthday to you",[](PrintTextA &out) {

	ConstStrA text("Happy ${celebration} dear ${name}, happy ${celebration} to you");
	StringA conv = StringA(convertString(PlaceholderConvert<char>(
			declPlaceholder<char>("name", "Jessie")
								("celebration","birthday")), text));
	out("%1") << conv;

});

}



