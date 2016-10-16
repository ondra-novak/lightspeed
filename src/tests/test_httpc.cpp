/*
* test_httc.cpp
*
*  Created on: 17. 7. 2016
*      Author: ondra
*/
#include "../lightspeed/base/containers/constStr.h"
#include "../lightspeed/base/framework/testapp.h"
#include "../lightspeed/base/containers/convertString.h"
#include "../lightspeed/base/streams/httpc.tcc"

namespace LightSpeed {

defineTest test_httpcRequestGet("httpc.requestGet", "", [](PrintTextA &out) {

	auto &&trgIter = out.getTarget().getTarget();

	auto outfn = [&trgIter](const ConstBin data) {
		trgIter.blockWrite(data);
	};

	HttpRequest<decltype(outfn)> req(outfn);

	req.setMethod("GET");
	req.setUrl("https://ondra:novak@example.com/path/file.ext?q=search+term");
	req.addHeader("Accept", "application/json");
	req.addHeader("If-None-Match", "aabbcc123bef3198");
	req.send();

});

defineTest test_httpcRequestPostKnownSize("httpc.requestPostKnownSize", "", [](PrintTextA &out) {

	auto &&trgIter = out.getTarget().getTarget();

	auto outfn = [&trgIter](const ConstBin data) {
		trgIter.blockWrite(data);
	};

	HttpRequest<decltype(outfn)> req(outfn);

	req.setMethod("POST");
	req.setUrl("https://ondra:novak@example.com/path/file.ext?q=search+term");
	req.addHeader("Content-Type", "text/plain");
	ConstStrA data = "Hello world";
	ConstBin datab(reinterpret_cast<const byte *>(data.data()), data.length());
	req.send(datab);

});

defineTest test_httpcRequestPostUnknownSize("httpc.requestPostUnknownSize", "", [](PrintTextA &out) {

	auto &&trgIter = out.getTarget().getTarget();

	auto outfn = [&trgIter](const ConstBin data) {
		trgIter.blockWrite(data);
	};

	HttpRequest<decltype(outfn)> req(outfn);

	req.setMethod("POST");
	req.setUrl("https://ondra:novak@example.com/path/file.ext?q=search+term");
	req.addHeader("Content-Type", "text/plain");
	ConstStrA data = "Hello world";
	ConstBin datab(reinterpret_cast<const byte *>(data.data()), data.length());
	req.beginBody();
	req.blockWrite(datab);	
	req.reset();

});

defineTest test_httpcResponseToEof("httpc.responseToEof", "200 text/plain This is test file\r\nOther line\r\n", [](PrintTextA &out) {

	const char *data =
		"HTTP/1.1 200 OK\r\n"
		"Server: Test server\r\n"
		"Content-Type: text/plain\r\n"
		"ETag: 90282bcaad398a\r\n"
		"\r\n"
		"This is test file\r\nOther line\r\n";

	auto infn = [&data]() -> int {
		if (*data) return *data++; else return -1;
	};
	HttpResponse<decltype(infn)> resp(infn);

	natural status = resp.open();
	AutoArrayStream<char> output;
	ConstStrA ctx = resp.getHeader("Content-Type");
	output.copy(resp);
	out("%1 %2 %3") << status << ctx << output.getArray();
});

defineTest test_httpcResponseWithLength("httpc.responseWithLength", "404 text/plain This is test file\r\n", [](PrintTextA &out) {

	const char *data =
		"HTTP/1.1 404 Not Found\r\n"
		"Server: Test server\r\n"
		"Content-Type: text/plain\r\n"
		"ETag: 90282bcaad398a\r\n"
		"Content-Length: 19\r\n"
		"\r\n"
		"This is test file\r\nOther line\r\n";

	auto infn = [&data]() -> int {
		if (*data) return *data++; else return -1;
	};
	HttpResponse<decltype(infn)> resp(infn);

	natural status = resp.open();
	AutoArrayStream<char> output;
	output.copy(resp);
	ConstStrA ctx = resp.getHeader("Content-Type");
	out("%1 %2 %3") << status << ctx << output.getArray();
});
defineTest test_httpcResponseChunked("httpc.responseChunked", "200 text/plain Wikipedia in\r\n\r\nchunks.", [](PrintTextA &out) {

	const char *data =
		"\r\n"
		"HTTP/1.1 200 Not Found\r\n"
		"Server: Test server\r\n"
		"Content-Type: text/plain\r\n"
		"Transfer-Encoding: chunked\r\n"
		"\r\n"
		"4\r\n"
		"Wiki\r\n"
		"5\r\n"
		"pedia\r\n"
		"E\r\n"
		" in\r\n"
		"\r\n"
		"chunks.\r\n"
		"0\r\n"
		"\r\n"
		"Blableblaebalbqeq";

	auto infn = [&data]() -> int {
		if (*data) return *data++; else return -1;
	};
	HttpResponse<decltype(infn)> resp(infn);

	natural status = resp.open();
	AutoArrayStream<char> output;
	output.copy(resp);
	ConstStrA ctx = resp.getHeader("Content-Type");
	out("%1 %2 %3") << status << ctx << output.getArray();
});


}