#include "../lightspeed/base/containers/constStr.h"
#include "../lightspeed/base/framework/testapp.h"
#include "../lightspeed/utils/json/jsonserializer.tcc"
namespace LightSpeedTest {

	using namespace LightSpeed;

	ConstStrA jsonSrc("{\"aaa\":10,\"bbb\":true,\"ccc\":false,\"array\":[11,232,1.213,3.2e21,1.2e-3,-12,-23.8232],\"null\":null,\"text\":\"Příšerně žluťoučký kůň úpěl ďábelské kódy\",\"emptyObj\":{}, \"emptyArr\":[]}");

	static void parser(PrintTextA &print) {
		JSON::PFactory f = JSON::create();
		JSON::Value v = f->fromString(jsonSrc);
		print("%1,%2,%3,%4,%5,%6,%7")
			<< v["aaa"]->getUInt()
			<< v["bbb"]->getBool()
			<< v["ccc"]->getBool()
			<< v["array"][3]->getFloat()
			<< v["array"][4]->getFloat()
			<< v["array"][6]->getFloat()
			<< v["null"]->isNull();
	}
	static void serialize(PrintTextA &print) {
		JSON::PFactory f = JSON::create();
		JSON::Value v = f->fromString(jsonSrc);
		JSON::serialize(v, print.nxChain(),false);
	}
	static void serializeEscaped(PrintTextA &print) {
		JSON::PFactory f = JSON::create();
		JSON::Value v = f->fromString(jsonSrc);
		JSON::serialize(v, print.nxChain(), true);
	}
	static void serializeCycleError(PrintTextA &print) {
		JSON::PFactory f = JSON::create();
		JSON::Value v = f->fromString(jsonSrc);
		JSON::Value arr = v["array"];
		arr->add(v);
		v->erase("text");
		JSON::serialize(v, print.nxChain(), false);
		v->erase("array");
	}

	static void numberToWide(PrintTextA &print) {
		JSON::PFactory f = JSON::create();
		JSON::Value v = f->newValue((natural)123);
		String tst1 = v->getString();
		String tst2 = v->getString();
		print("%1,%2") << tst1 << tst2;
	}

	static void testEqual(PrintTextA &print, JSON::Value a, JSON::Value b) {
		if (a == b) {
			if (*a == *b) print("S");
			else print("X");
		} else {
			if (*a == *b) print("E");
			else print("N");
		}
	}

	static void sharePrimitives(PrintTextA &print) {
		JSON::PFactory f = JSON::create();
		testEqual(print,f(true),f(true));
		testEqual(print,f(false),f(false));
		testEqual(print,f(null),f(null));
		testEqual(print,f(0),f(0));
		testEqual(print,f(0.0),f(0));
		testEqual(print,f(ConstStrA("")),f(ConstStrA("")));
		testEqual(print,f(ConstStrW(L"")),f(ConstStrW(L"")));
		testEqual(print,f(ConstStrA("")),f(ConstStrW(L"")));
		testEqual(print,f.array(),f.array());
		testEqual(print,f.object(),f.object());
		JSON::Value p = f(true);
		ConstStringT<JSON::Value> v(&p,1);
		testEqual(print,f(v),f(v));
		testEqual(print,f(10),f(20));
		testEqual(print,f(ConstStrA("")),f(ConstStrA("ahoj")));
		testEqual(print,f(true),f(1));
		testEqual(print,f(false),f(0));
		testEqual(print,f(false),f(""));
		testEqual(print,f(10),f(10));
		testEqual(print,f(10.1),f(10.1));
		testEqual(print,f("Ahoj"),f("Ahoj"));
	}

	defineTest test_parser("json.parser", "10,1,0,3200000000000000000000.000000,0.001200,-23.823200,1", &parser);
	defineTest test_serialize("json.serialize", "{\"aaa\":10,\"array\":[11,232,1.213000,3200000000000000000000.000000,0.001200,-12,-23.823200],\"bbb\":true,\"ccc\":false,\"emptyArr\":[],\"emptyObj\":{},\"null\":null,\"text\":\"Příšerně žluťoučký kůň úpěl ďábelské kódy\"}", &serialize);
	defineTest test_serialize_escaped("json.serialize_escaped", "{\"aaa\":10,\"array\":[11,232,1.213000,3200000000000000000000.000000,0.001200,-12,-23.823200],\"bbb\":true,\"ccc\":false,\"emptyArr\":[],\"emptyObj\":{},\"null\":null,\"text\":\"P\\u0159\\u00ED\\u0161ern\\u011B \\u017Elu\\u0165ou\\u010Dk\\u00FD k\\u016F\\u0148 \\u00FAp\\u011Bl \\u010F\\u00E1belsk\\u00E9 k\\u00F3dy\"}", &serializeEscaped);
	defineTest test_serialize_cycle("json.serialize_cycle_error", "{\"aaa\":10,\"array\":[11,232,1.213000,3200000000000000000000.000000,0.001200,-12,-23.823200,{\"error\":\"<infinite_recursion>\"}],\"bbb\":true,\"ccc\":false,\"emptyArr\":[],\"emptyObj\":{},\"null\":null}", &serializeCycleError);
	defineTest test_number_to_wide("json.number_to_wide", "123,123", &numberToWide);
	defineTest test_sharePrimitives("json.sharePrimitives", "SSSSSSSSEEENNNNNEEE", &sharePrimitives);
}
