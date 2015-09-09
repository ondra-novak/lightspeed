/*
 * tcpHandler.cpp
 *
 *  Created on: 23. 2. 2015
 *      Author: ondra
 */

#include "tcpHandler.h"

#include "../text/textParser.tcc"
#include "../../utils/queryParser.h"
#include "../exceptions/fileExceptions.h"
#include "../streams/netio.h"
namespace LightSpeed {

PInOutStream TcpHandler::openSeqFile(ConstStrW fname, FileOpenMode, OpenFlags::Type) {

	StringA netname = String::getUtf8(fname);
	TextParser<char> parser;
	if (!parser("tcp://%[]0-9a-zA-Z.-:[]1/%", netname)) {
		throw FileOpenError(THISLOCATION,2,netname);
	}

	ConstStrA addr = parser[1].str();

	QueryParser qparser(netname);

	natural connectTimeout = 5000;
	natural iotimeout = 30000;

	bool server = false;

	while (qparser.hasItems()) {
		QueryField itm = qparser.getNext();
		natural val;
		if (!parseUnsignedNumber(itm.value.getFwIter(),val,10)) {
			throw FileOpenError(THISLOCATION,2,netname);
		}
		if (itm.name == "server") {
			server = val != 0;
		}
		else if (itm.name == "connect_timeout") {
			connectTimeout = val;
		}
		else if (itm.name == "io_timeout") {
			iotimeout = val;
		}
	}

	NetworkAddress naddr(addr,80);
	NetworkStreamSource nss(naddr,1,connectTimeout,iotimeout,server?StreamOpenMode::active:StreamOpenMode::passive);
	PNetworkStream stream = nss.getNext();
	return stream.get();
}

void TcpHandler::unsupported() const {
	throwUnsupportedFeature(THISLOCATION,this,"tcp:// protocol doesn't support this feature");
}

} /* namespace LightSpeed */
