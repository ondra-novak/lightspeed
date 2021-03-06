/*
 * SimpleHTTPServer.h
 *
 *  Created on: 20.6.2012
 *      Author: ondra
 */

#ifndef SIMPLEHTTPSERVER_H_
#define SIMPLEHTTPSERVER_H_

#include "../lightspeed/base/framework/TCPServer.h"
#include "../lightspeed/base/framework/serviceapp.h"

namespace LightSpeedTest {

using namespace LightSpeed;

class SimpleHTTPServer: public ITCPServerConnHandler {
public:

	SimpleHTTPServer(String rootPath, String indexFile);

	typedef Map<StringAI, StringA> Header;

	virtual void methodGET(ConstStrA vpath, const Header &header, SeqFileOutput &out);
	virtual void methodPOST(ConstStrA vpath, const Header &header, natural contentLength, SeqFileInput &in, SeqFileOutput &out);

	static void sendHeader(SeqFileOutput &out, int status, ConstStrA statusMsg, const Header &header);
	static void sendHeader(SeqFileOutput &out, const Header &header);


protected:
	String rootPath;
	String indexFile;

	virtual Command onDataReady(const PNetworkStream &stream, ITCPServerContext *context) throw() {return onData(stream,context)?cmdWaitRead:cmdRemove;}
	virtual Command onWriteReady(const PNetworkStream &, ITCPServerContext *) throw() {return cmdRemove;}
	virtual Command onTimeout(const PNetworkStream &, ITCPServerContext *) throw () {return cmdRemove;}
	virtual Command onUserWakeup(const PNetworkStream &, ITCPServerContext *) throw() {return cmdRemove;}
	virtual void onDisconnectByPeer(ITCPServerContext *) throw () {}
	virtual ITCPServerContext *onIncome(const NetworkAddress &addr) throw() {return onConnect(addr);}
	virtual Command onAccept(ITCPServerConnControl *, ITCPServerContext *) {return cmdWaitRead;}





	static bool loadHeader(Header &hdr, ScanTextA &reader);

	virtual bool onData(const PNetworkStream &stream, ITCPServerContext *context)  throw();
	virtual ITCPServerContext *onConnect(const NetworkAddress &addr) throw();

	static void sendHeader2(SeqFileOutput &out, int status, ConstStrA statusMsg, const Header &header);


};

class SimpleHTTPServerService: public ServiceApp {
public:

	AllocPointer<SimpleHTTPServer> httpserver;
	AllocPointer<TCPServer> tcpserver;

	virtual integer initService(const Args & args, SeqFileOutput serr);
    virtual integer startService();
	virtual bool stop();

	natural port;

};

} /* namespace LightSpeed */
#endif /* SIMPLEHTTPSERVER_H_ */
