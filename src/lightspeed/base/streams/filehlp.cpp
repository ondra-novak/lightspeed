/*
 * filehlp.cpp
 *
 *  Created on: 30.7.2013
 *      Author: ondra
 */


#include "fileio_ifc.h"
#include "../../base/text/textParser.tcc"
#include "../exceptions/invalidParamException.h"

namespace LightSpeed {

void IHTTPSettings::setProxy(ProxyMode md, ConstStrA addrport) {

	if (md != pmManual) {
		setProxy(md,String(),0);
	} else {
		TextParser<char, SmallAlloc<1024> > parser;
		if (parser("%1:%u2",addrport)) {
			setProxy(md,parser[1].str(),(natural)parser[2]);
		} else {
			throw InvalidParamException(THISLOCATION,2,"Proxy address must be in format <domain>:<port>");
		}
	}


}


}
