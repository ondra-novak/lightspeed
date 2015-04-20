#include <unistd.h>
#include "linuxhttp.h"
#include <sys/select.h>
#include "../text/textParser.tcc"
#include "../text/textFormat.tcc"
#include "../exceptions/fileExceptions.h"
#include "../exceptions/httpStatusException.h"
#include "../interface.tcc"
#include "linuxFdSelect.h"
#include "../debug/dbglog.h"

namespace LightSpeed {


LinuxHttpStream::LinuxHttpStream(String url, const HTTPSettings& defaultSettins)
:LinuxHTTPSettings(settings),url(url),settings(defaultSettins),state(stIdle),inputStream(nil),outputStream(nil),wgetProc("/usr/bin/wget")
,noredir(false),noexecpt(false)
{
}

natural LinuxHttpStream::read(void* buffer, natural size) {
	if (state != stReading) sendRequest();
	return inputStream.blockRead(buffer,size,false);
}

natural LinuxHttpStream::write(const void* buffer, natural size) {
	if (state == stIdle) preparePost();
	else if (state != stWriting)
		throwWriteIteratorNoSpace(THISLOCATION,typeid(*this));

	return outputStream.blockWrite(buffer,size,false);
}

natural LinuxHttpStream::peek(void* buffer, natural size) const {
	if (state != stReading) const_cast<LinuxHttpStream *>(this)->sendRequest();
	return inputStream.blockRead(buffer,size,false);
}

bool LinuxHttpStream::canRead() const {
	if (state != stReading) const_cast<LinuxHttpStream *>(this)->sendRequest();
	return inputStream.hasItems();
}

bool LinuxHttpStream::canWrite() const {
	return (state == stIdle || state == stWriting);
}

void LinuxHttpStream::flush() {
	if (state == stWriting) outputStream.flush();
}

natural LinuxHttpStream::dataReady() const {
	if (state != stReading) return 0;
	else return inputStream.getHandle()->dataReady();
}


IHTTPStream& LinuxHttpStream::setMethod(ConstStrA method) {
	this->method = method;
	return *this;
}

IHTTPStream& LinuxHttpStream::setHeader(ConstStrA headerName, ConstStrA headerValue) {
	sendHeader.replace(headerPool.add(headerName),headerPool.add(headerValue));
	return *this;
}


IHTTPStream& LinuxHttpStream::cancel() {
	if (state == stReading) wgetProc.stop(true);
	return *this;
}

StringA LinuxHttpStream::getReplyHeaders() {
	return replyHdrPool;
}

ConstStrA LinuxHttpStream::getHeader(ConstStrA field) {
	const ConstStrA *r = replyHeader.find(field);
	if (r == 0) return ConstStrA();
	else return *r;
}

bool LinuxHttpStream::enumHeaders(IEnumHeaders& enumHdr) {
	for (ReplyHeader::Iterator iter = replyHeader.getFwIter(); iter.hasItems();) {
		const ReplyHeader::Entity &e = iter.getNext();
		if (enumHdr(e.key,e.value)) return true;
	}
	return false;
}

bool LinuxHttpStream::inConnected() const {
	return state == stReading;
}

IHTTPStream& LinuxHttpStream::disableRedirect() {
	noredir = true; return *this;
}

IHTTPStream& LinuxHttpStream::disableException() {
	noexecpt = true; return *this;
}

IHTTPStream& LinuxHttpStream::connect() {
	if (state != stReading) sendRequest();
	return *this;
}

natural LinuxHttpStream::getStatusCode() {
	if (state != stReading) sendRequest();
	return statusCode;
}

static PTemporaryFile cookieStore;

void LinuxHttpStream::cropLine(ConstStrA& hdrline) {
	while (!hdrline.empty() && isspace(hdrline.at(0)))
		hdrline = hdrline.offset(1);
	while (!hdrline.empty() && isspace(hdrline.at(hdrline.length() - 1)))
		hdrline = hdrline.crop(0, 1);
}

void LinuxHttpStream::parseReplyHeader() {
//	LogObject lg(THISLOCATION);
	TextParser<char, StaticAlloc<4096> > parser;
	for (AutoArray<char>::SplitIterator iter = replyHdrPool.split('\n');
			iter.hasItems();) {
		ConstStrA hdrline = iter.getNext();
		cropLine(hdrline);
//		lg.debug("HTTP response -  %1") << hdrline;
		if (parser("HTTP/%f1 %u2 %3",hdrline)) {
			statusCode = parser[2];
			statusMsg = parser[3].str();
			replyHeader.clear();
		} else {
			natural div = hdrline.find(':');
			if (div != naturalNull) {
				ConstStrA hdr = hdrline.head(div);
				ConstStrA value = hdrline.offset(div + 1);
				cropLine(hdr);
				cropLine(value);
				replyHeader.replace(hdr, value);
			}
		}

	}

}


void LinuxHttpStream::readHeader(SeqFileInput& wgetin, SeqFileInput& wgeterr) {
	int wgetin_fd, wgeterr_fd;
	wgetin.getHandle()->getIfc<IFileExtractHandle>().getHandle(&wgetin_fd,
			sizeof(wgetin_fd));
	wgeterr.getHandle()->getIfc<IFileExtractHandle>().getHandle(&wgeterr_fd,
			sizeof(wgeterr_fd));

	LinuxFdSelect fdselect;

	fdselect.set(wgetin_fd,INetworkResource::waitForInput,0,naturalNull);
	fdselect.set(wgeterr_fd,INetworkResource::waitForInput,0,naturalNull);


	bool rep;
	bool data = false;
	do {
		rep = false;

		const LinuxFdSelect::FdInfo &fs = fdselect.getNext();
		if (fs.fd == wgeterr_fd) {
			char buff[256];
			int n = ::read(wgeterr_fd, buff, 256);
			if (n == 0) {
				rep = false;
			} else {
				replyHdrPool.append(ConstStrA(buff, n));
				fdselect.set(wgetin_fd,INetworkResource::waitForInput,0,naturalNull);
				rep = true;
				data = false;
			}
		} else if (fs.fd == wgetin_fd) {
			rep = !data;
			data = true;
		}

	} while (rep);
}

void LinuxHttpStream::sendRequest() {
	IFileIOServices& svc = IFileIOServices::getIOServices();

	if (settings.ipver == 4) wgetProc.arg("-4");
	if (settings.ipver == 6) wgetProc.arg("-6");
	//prev state is writing?
	if (state == stWriting) {
		//yes, there is written data
		//close temporary file
		tempBuffer->close();
		//close handle
		outputStream = SeqFileOutput(nil);
		//attach temporary file to wget request
		wgetProc.arg("--post-file").arg(tempBuffer->getFilename());
		method = "POST";
	} else if (state == stIdle && method == "POST") {
		preparePost();
		return sendRequest();
	}
	//are cookies enabled
	if (settings.cookiesEnabled) {
		if (cookieStore == nil) {
			//create cookieStore
			cookieStore = svc.createTempFile(L"cookies", true);
			cookieStore->close();
		}
		//allow loading and storing cookies
		wgetProc.arg("--load-cookies").arg(cookieStore->getFilename()).arg(
				"--save-cookies").arg(cookieStore->getFilename()).arg(
				"--keep-session-cookies");
	}



	ProcessEnv env;
	TextFormatBuff<wchar_t, StaticAlloc<1024> > fmt;
	//in case that proxy is turned off
	String proxyAddr;
	if (settings.proxyMode == pmDirect) {
		//append --no-proxy argument
		wgetProc.arg("--no-proxy");
	} else if (settings.proxyMode == pmManual) {
		env = wgetProc.getEnv();
		//for manual proxy
		fmt("http://%1:%2") << settings.proxyAddr << settings.proxyPort;
		//rewrite environment strings
		proxyAddr = fmt.write();;
		env(L"http_proxy") = proxyAddr;
		env(L"https_proxy") = proxyAddr;
		wgetProc.setEnv(&env);
	}

	if (!settings.userAgent.empty()) {
		wgetProc.arg("--user-agent");
		wgetProc.arg(settings.userAgent);
	}
	//proces user headers
	for(Header::Iterator iter = sendHeader.getFwIter(); iter.hasItems();) {
		wgetProc.arg("--header");
		const Header::Entity &ent = iter.getNext();
		fmt("%1: %2") << ent.key << ent.value;
		wgetProc.arg(ConstStrW(fmt.write()));
	}

	//specify timeout
	if (settings.defaultTimeout != naturalNull) {
		fmt("%1.%{03}2") << settings.defaultTimeout / 1000
				<< settings.defaultTimeout % 1000;
		wgetProc.arg("-T").arg(ConstStrW(fmt.write()));
	}
	//if noredir, disable redirection
	if (noredir) wgetProc.arg("--max-redirect").arg(natural(0));
	//final sequence - output to stdout, server response to stderr and be quiet
	wgetProc.arg("-O").arg("-").arg("-S").arg("-q");
	///send url as final argument
	wgetProc.arg(url);
	///prepare output streams
	SeqFileInput wgetin(nil), wgeterr(nil);
	///registers streams
	wgetProc.stdErr(wgeterr).stdOut(wgetin);

	state = stReading;

	///start downloading
	wgetProc.start();


	///read all headers
	readHeader(wgetin, wgeterr);

	if (replyHdrPool.empty()) {
		integer res = wgetProc.join();
		if (res == 4)
			throw FileMsgException(THISLOCATION, ENOENT, url,
					"Can't connect server (network error)");
		if (res == 5)
			throw FileMsgException(THISLOCATION, EPERM, url,
					"Invalid SSL certificate");
		if (res == 6)
			throw FileMsgException(THISLOCATION, EACCES, url,
					"Invalid SSL certificate");
		if (res == 7)
			throw FileMsgException(THISLOCATION, EPROTO, url, "Protocol error");
		else
			throw FileIOError(THISLOCATION, res, url);
	}
	parseReplyHeader();
	if (method != "HEAD") inputStream = wgetin;

	if (!noexecpt && statusCode / 100 != 2)
		throw HttpStatusException(THISLOCATION,url,statusCode,ConstStrW());

}

void LinuxHttpStream::preparePost() {
	IFileIOServices &svc = IFileIOServices::getIOServices();
	tempBuffer = svc.createTempFile(L"post",true);
	outputStream = SeqFileOutput(tempBuffer->getStream());
	state = stWriting;
}


LinuxHttpStream::~LinuxHttpStream() {
	try {
		if (wgetProc.isRunning()) {
			wgetProc.stop(false);
			if (wgetProc.join(Timeout(2000)) == false)
				wgetProc.stop(true);
		}
	} catch (...) {

	}
}

size_t LinuxHttpStream::getHandle(void *buffer, size_t bufferSize) {
	if (state != stReading) sendRequest();
	return inputStream.getHandle()->getIfc<IFileExtractHandle>().getHandle(buffer,bufferSize);


}

void LinuxHttpStream::closeOutput() {
	if (state != stReading) {
		flush();
		sendRequest();
	}
}


PInOutStream LinuxHttpHandler::openSeqFile(ConstStrW fname, FileOpenMode , OpenFlags::Type ) {
   		return new LinuxHttpStream(fname,httpSettings);
}

PRndFileHandle LinuxHttpHandler::openRndFile(ConstStrW , FileOpenMode , OpenFlags::Type ) {
	throwUnsupportedFeature(THISLOCATION,this,"");throw;
}

bool LinuxHttpHandler::canOpenFile(ConstStrW , FileOpenMode ) const {
	throwUnsupportedFeature(THISLOCATION,this,"");throw;
}

PFolderIterator LinuxHttpHandler::openFolder(ConstStrW ) {
	throwUnsupportedFeature(THISLOCATION,this,"");throw;
}

PFolderIterator LinuxHttpHandler::getFileInfo(ConstStrW ) {
	throwUnsupportedFeature(THISLOCATION,this,"");throw;
}

void LinuxHttpHandler::createFolder(ConstStrW , bool ) {
	throwUnsupportedFeature(THISLOCATION,this,"");throw;
}

void LinuxHttpHandler::removeFolder(ConstStrW , bool ) {
	throwUnsupportedFeature(THISLOCATION,this,"");throw;
}

void LinuxHttpHandler::copy(ConstStrW , ConstStrW , bool ) {
	throwUnsupportedFeature(THISLOCATION,this,"");throw;
}

void LinuxHttpHandler::move(ConstStrW , ConstStrW , bool ) {
	throwUnsupportedFeature(THISLOCATION,this,"");throw;
}

void LinuxHttpHandler::link(ConstStrW , ConstStrW,bool ) {
	throwUnsupportedFeature(THISLOCATION,this,"");throw;

}

void LinuxHttpHandler::remove(ConstStrW ) {
	throwUnsupportedFeature(THISLOCATION,this,"");throw;
}

PMappedFile LinuxHttpHandler::mapFile(ConstStrW , FileOpenMode ) {
	throwUnsupportedFeature(THISLOCATION,this,"");throw;

}

}
