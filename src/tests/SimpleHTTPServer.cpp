/*
 * SimpleHTTPServer.cpp
 *
 *  Created on: 20.6.2012
 *      Author: ondra
 */

#include "SimpleHTTPServer.h"
#include "../lightspeed/base/exceptions/fileExceptions.h"
#include "../lightspeed/base/text/textstream.tcc"
#include "../lightspeed/base/streams/fileiobuff.tcc"

namespace LightSpeedTest {




SimpleHTTPServer::SimpleHTTPServer(String rootPath,
		String indexFile):rootPath(rootPath),indexFile(indexFile)  {

}

void SimpleHTTPServer::methodGET(ConstStrA vpath, const Header& ,SeqFileOutput& out) {

	try {

		if (vpath.empty()) {
			Header hdr;
			hdr.insert("Location","/");
			sendHeader(out,301,"Redirect",hdr);
			return;
		}

		if (vpath[0] != '/') {
			Header hdr;
			hdr.insert("Location",StringA(ConstStrA("/") + vpath));
			sendHeader(out,301,"Redirect",hdr);
			return;

		}

		String path = rootPath + String(vpath);
		if (vpath.tail(1) != ConstStrA("/")) {
				PFolderIterator finfo = IFileIOServices::getIOServices().getFileInfo(path);
				if (finfo->type & IFolderIterator::directory) {
					Header hdr;
					hdr.insert("Location",StringA(vpath) + ConstStrA("/"));
					sendHeader(out,301,"Redirect",hdr);
					return;

				}
		} else {
			path = path + indexFile;
		}

		natural k = path.findLast('.');
		ConstStrW ext;
		ConstStrA ctxt;
		if (k != naturalNull) ext = path.offset(k+1);
		if (ext == ConstStrW(L"htm") || ext == ConstStrW(L"html"))
			ctxt = ConstStrA("text/html");
		else if (ext == ConstStrW(L"txt"))
			ctxt = ConstStrA("text/plain");
		else if (ext == ConstStrW(L"jpg"))
			ctxt = ConstStrA("image/jpeg");
		else if (ext == ConstStrW(L"gif"))
			ctxt = ConstStrA("image/gif");
		else if (ext == ConstStrW(L"png"))
			ctxt = ConstStrA("image/png");
		else if (ext == ConstStrW(L"css"))
			ctxt = ConstStrA("text/css");
		else if (ext == ConstStrW(L"js"))
			ctxt = ConstStrA("text/javascript");
		else
			ctxt = ConstStrA("application/octet-stream");

		Header hdr;
		hdr.insert("Content-type",ctxt);

		SeqFileInBuff<> infile(path,0);
		AutoArray<byte,StaticAlloc<2048> > buffer;
		buffer.resize(2048);
		sendHeader(out,hdr);
		out.blockCopy(infile,buffer);

	} catch (const FileIOException &e) {
		Header hdr;
		sendHeader(out,404,"Not found",hdr);
	}


}

void SimpleHTTPServer::methodPOST(ConstStrA, const Header& ,natural contentLength, SeqFileInput& in, SeqFileOutput& out) {

	Header hdr;
	for (natural i = 0; i < contentLength; i++) {
		in.skip();
	}

	sendHeader(out,404,"Not found",hdr);
}


class DumpStream: public IOutputStream {
public:
	DumpStream(IOutputStream &bkgHandle):bkgHandle(bkgHandle) {}
	IOutputStream &getBkgHandle() {return bkgHandle;}

    virtual natural read(void *,  natural ) {return 0;}
    virtual natural write(const void *,  natural size) {return size;}
	virtual natural peek(void *, natural ) const {return 0;}
	virtual bool canRead() const {return false;}
	virtual bool canWrite() const {return true;}
	virtual void flush() {}
	virtual natural dataReady() const {return 0;}
	virtual void closeOutput() {}



protected:
	IOutputStream &bkgHandle;


};

bool SimpleHTTPServer::onData(const PNetworkStream& stream, ITCPServerContext* ) throw() {

	NetworkStream<> mystream(stream);
	ScanTextA reader(mystream);
	PrintTextA print(mystream);
	reader.setNL(ConstStrA("\r\n"));
	print.setNL(ConstStrA("\r\n"));
	if (reader("%[a-zA-Z]1 %2 HTTP/%[0-9.]3 \n%")) {
		StringA method = reader[1].str();
		StringA vpath = reader[2].str();
//		StringA ver = reader[3];

		try {
			Header header;

			if (!loadHeader(header,reader)) {
				print("HTTP/1.0 400 Syntax error in header\n");
			} else if (method == ConstStrA("GET")) {
				methodGET(vpath,header,mystream);
			} else if (method == ConstStrA("HEAD")) {
				DumpStream dump(*mystream.getBufferedOutputStream());dump.setStaticObj();
				SeqFileOutput dumpout(&dump);
				methodGET(vpath,header,dumpout);
			} else if (method == ConstStrA("POST")) {
				const StringA *szstr = header.find("Content-length");
				if (szstr == 0) {
					print("HTTP/1.0 400 Content-length required\n");
				} else {
					natural val;
					if (parseUnsignedNumber(szstr->getFwIter(),val,10) == false)
						print("HTTP/1.0 400 Syntax error in header\n");
					else
						methodPOST(vpath,header,val,mystream,mystream);
				}
			} else {
				print("HTTP/1.0 400 Unknown method\n");
			}

		} catch (std::exception &e) {
			print("HTTP/1.0 500 %1\n")  << e.what();
		}


	} else {
		print("HTTP/1.0 400 Bad Request\n");
	}
	return false;


}

bool SimpleHTTPServer::loadHeader(Header& hdr, ScanTextA& reader) {

	hdr.clear();
	StringAI lastHdr;
	do {
		if (reader(" \n%")) return true;
		if (reader(" %1 : %2 \n%")) {
			ConstStrA key = reader[1];
			ConstStrA value = reader[2];
			lastHdr = key;
			hdr(lastHdr) = value;
		} else if (reader("\t %1 \n%")) {
			ConstStrA value = reader[1];
			hdr(lastHdr) = hdr[lastHdr] + ConstStrA(" ") + value;
		} else {
			return false;
		}
	} while (true);
	throw;
}


void SimpleHTTPServer::sendHeader2(SeqFileOutput& out, int status,
									ConstStrA statusMsg, const Header& header) {

	PrintTextA print(out);
	print.setNL(ConstStrA("\r\n"));

	print("HTTP/1.0 %1 %2\n") << status << statusMsg;
	for(Header::Iterator iter = header.getFwIter(); iter.hasItems();) {
		const Header::Entity &e = iter.getNext();
		print("%1: %2\n") << e.key << e.value;
	}

	print("\n");

}

void SimpleHTTPServer::sendHeader(SeqFileOutput& out, int status,
									ConstStrA statusMsg, const Header& header) {
	DumpStream *strm = dynamic_cast<DumpStream *>(out.getHandle().get());
	if (strm) {
		SeqFileOutput o(strm);
		sendHeader2(o,status,statusMsg,header);
	} else {
		sendHeader2(out,status,statusMsg,header);
	}
}

void SimpleHTTPServer::sendHeader(SeqFileOutput& out, const Header& header) {
	sendHeader(out,200,"OK",header);
}

ITCPServerContext* SimpleHTTPServer::onConnect(
		const NetworkAddress& ) throw () {
	return 0;
}
/* namespace LightSpeed */


integer SimpleHTTPServerService::initService(const Args& args, SeqFileOutput serr) {
	PrintTextA print(serr);
	if (args.length() < 2) {
		print("Usage: port documentRoot [indexFile]\n");
	} else {
		if (!parseUnsignedNumber(args[0].getFwIter(),port,10)) {
			print("Port must be number\n");
		} else {
			ConstStrW index;
			if (args.length()>2) index = args[2];
			httpserver = new SimpleHTTPServer(args[1],index);
			tcpserver = new TCPServer(*httpserver.get(),32);
			//probe port
			NetworkStreamSource probePort(port,naturalNull,naturalNull,naturalNull,false);
			//probe probe
			return ServiceApp::initService(args,serr);
		}
	}
	return 1;
}

integer SimpleHTTPServerService::startService() {
	tcpserver->start(port,false,naturalNull);
	return ServiceApp::startService();
}

bool SimpleHTTPServerService::stop() {
	tcpserver->stop();
	return ServiceApp::stop();

}


}
