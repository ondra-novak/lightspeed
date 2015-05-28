#include "winpch.h"
#include "winhttp.h"
#include "..\exceptions\fileExceptions.h"
#include "..\text\textParser.tcc"
#include "..\text\textFormat.tcc"
#include "../containers/autoArray.tcc"
#include "../interface.tcc"
#include "../exceptions/httpStatusException.h"

namespace LightSpeed {

template IHTTPStream &IInterface::getIfc<IHTTPStream>(void);
template const IHTTPStream &IInterface::getIfc<IHTTPStream>(void) const;
template Pointer<IHTTPStream>IInterface::getIfcPtr<IHTTPStream>(void);
template Pointer<const IHTTPStream>IInterface::getIfcPtr<IHTTPStream>(void) const;

#pragma comment (lib, "Wininet.lib")




WinHttpStream::WinHttpStream( String url, const HTTPSettings &settings )
:WinHTTPSettings(this->settings),hInternet(0),hConnect(0),hHTTPConn(0),exceptionDisabled(false),redirDisabled(false)
,url(url),settings(settings),method("GET")
,curState(stIdle),tmaction(0)
{
}

WinHttpStream::~WinHttpStream()
{
	if (!postBuffer.empty() && !std::uncaught_exception()) setState(stReadResponse);
	timeoutThr.stop();
	if (hHTTPConn) InternetCloseHandle(hHTTPConn);
	if (hConnect) InternetCloseHandle(hConnect);
	if (hInternet) InternetCloseHandle(hInternet);
}


IHTTPStream & WinHttpStream::setMethod( ConstStrA method )
{
	this->method = method;
	return *this;
}

IHTTPStream & WinHttpStream::setHeader( ConstStrA headerName, ConstStrA headerValue )
{
	hdrmap.replace(headerName,headerValue);
	return *this;
}



IHTTPStream & WinHttpStream::cancel()
{
	if (hHTTPConn) {
		InternetCloseHandle(hHTTPConn);
		hHTTPConn = 0;
	}
	return *this;
}

StringA WinHttpStream::getReplyHeaders()
{
	setState(stReadResponse);
	AutoArray<char> buffer;
	buffer.resize(1000);
	DWORD size = (DWORD)buffer.length();
	while (!HttpQueryInfoA(hHTTPConn,HTTP_QUERY_RAW_HEADERS_CRLF,
		buffer.data(),&size,0)) {
			DWORD res = GetLastError();
			if (res == ERROR_INSUFFICIENT_BUFFER) {
				buffer.resize(size);
			} else {
				throw ErrNoWithDescException(THISLOCATION,GetLastError(),
					"Cannot retrieve headers from the request");
			}
	}
	buffer.resize(size);
	return buffer;
}


IHTTPStream & WinHttpStream::disableRedirect()
{
	redirDisabled = true;
	return *this;
}

IHTTPStream & WinHttpStream::disableException()
{
	exceptionDisabled= true;
	return *this;

}

IHTTPStream & WinHttpStream::connect()
{
	if (method == ConstStrA("GET") || method == ConstStrA("HEAD"))
		setState(stReadResponse);
	else
		setState(stWriteRequest);
	return *this;
}

LightSpeed::natural WinHttpStream::getStatusCode()
{
	setState(stReadResponse);
	DWORD statusCode = 0;
	DWORD buffSz = sizeof(statusCode);
	DWORD index = 0;
	if (HttpQueryInfo(hHTTPConn,HTTP_QUERY_STATUS_CODE|HTTP_QUERY_FLAG_NUMBER,
		&statusCode,&buffSz,&index) == FALSE)
		throw FileMsgException(THISLOCATION,GetLastError(),
		url,
		"Unable to retrieve status code. HttpQueryInfo has failed");
	else 
		return statusCode;
}

void WinHttpStream::setState( State newState )
{
	while (newState > curState) {
		if (curState == stIdle) {
			initRequest();
			curState = stWriteRequest;
		} else if (curState == stWriteRequest) {
			sendRequest();
			curState = stReadResponse;
		} else {
			break;
		}
	}
}

natural WinHttpStream::read( void *buffer, natural size )
{
	setState(stReadResponse);
	if (!peekBuffer.empty()) {
		natural r = peekBuffer.length();
		if (r <= size) {
			memcpy(buffer,peekBuffer.data(),r);
			peekBuffer.clear();
			return r;
		} else {
			memcpy(buffer,peekBuffer.data(),size);
			peekBuffer.erase(0,size);
			return size;

		}
	} else {
		return readInternal(buffer, size);
	}
}

LightSpeed::natural WinHttpStream::peek( void *buffer, natural size ) const
{
	const_cast<WinHttpStream *>(this)->setState(stReadResponse);
	if (size <= peekBuffer.length()) {
		memcpy(buffer,peekBuffer.data(),size);
		return size;
	}
	natural rm = size - peekBuffer.length();
	AutoArray<byte, StaticAlloc<256> > peekBuffer2;
	peekBuffer2.resize(rm);
	natural r = const_cast<WinHttpStream *>(this)->readInternal(peekBuffer2.data(),peekBuffer2.length());
	peekBuffer2.resize(r);
	peekBuffer.append(peekBuffer2);
	return peek(buffer,r);
}

LightSpeed::natural WinHttpStream::readInternal( void * buffer, natural size ) 
{
	DWORD rd;
	BOOL res = InternetReadFile(hHTTPConn,buffer,(DWORD)size,&rd);
	DWORD err = GetLastError();
	if (res == FALSE) {
		throw FileMsgException(THISLOCATION,err,url,"Read failed");
	}
	return rd;
}

LightSpeed::natural WinHttpStream::write( const void *buffer, natural size )
{
	postBuffer.append(ArrayRef<const byte>(reinterpret_cast<const byte *>(buffer),size));
	if (method == ConstStrA("GET")) method = ConstStrA("POST");
/*



	if (curState == stIdle) {
		setState(stWriteRequest);
	}

	TMWatch _w(*this);
	DWORD rd;
	BOOL res = InternetWriteFile(hHTTPConn,buffer,size,&rd);
	DWORD err = GetLastError();
	if (res == FALSE) {
		throw FileMsgException(THISLOCATION,err,url,"Write failed");
	}*/
	return size;	
}

bool WinHttpStream::canRead() const
{
	byte b;
	return peek(&b,1) != 0;
}

bool WinHttpStream::canWrite() const
{
	return (method != ConstStrA("GET") && method != ConstStrA("HEAD")) || hHTTPConn == 0;
}

void WinHttpStream::flush()
{
	
}

natural WinHttpStream::dataReady() const
{
	return 0;
}

void WinHttpStream::initRequest()
{
	TextParser<wchar_t> parser;
	if (!parser(L"%1://%[*-_.a-zA-Z0-9:@]2%%[/](*)*3",url))
		throw FileOpenError(THISLOCATION,ERROR_FILE_NOT_FOUND,url);

	String protocol = parser[1].str();
	String hostident = parser[2].str();
	String::SplitIterator hostidentsplit = hostident.split('@');
	String ident;
	String domain;
	domain = hostidentsplit.getNext();
	while (hostidentsplit.hasItems()) {
		ident = ident + domain + ConstStrW('@');
		domain = hostidentsplit.getNext();
	}
	String path = parser[3].str();
	natural port;
	String username;
	String password;
	bool secure;

	if (parser( L"%1:%u2",domain)) {
		domain = parser[1].str();
		port = parser[2];
	} else if (protocol == ConstStrW(L"http")) {
		port = INTERNET_DEFAULT_HTTP_PORT;
		secure = false;
	} else if (protocol == ConstStrW(L"https")) {
		port = INTERNET_DEFAULT_HTTPS_PORT;
		secure = true;
	} else
		throw FileOpenError(THISLOCATION,ERROR_NOT_FOUND,url);

	if (!ident.empty()) {
		if (parser(L"%1:%2@",ident))  {
			username = parser[1].str();
			password = parser[2].str();
		} else {
			throw FileMsgException(THISLOCATION,0,url,"Invalid identification field in the url");
		}
	}
	DWORD accessType;
	switch (settings.proxyMode) {
	case pmManual:  accessType = INTERNET_OPEN_TYPE_PROXY;
	case pmDirect:  accessType = INTERNET_OPEN_TYPE_DIRECT;
	case pmAuto:  accessType = INTERNET_OPEN_TYPE_PRECONFIG;
	}

	TextFormatBuff<wchar_t> fmt;

	String proxyName;
	if (accessType == INTERNET_OPEN_TYPE_PROXY) {
		fmt(L"%1:%2") << settings.proxyAddr << settings.proxyPort;
		proxyName = fmt.write();
	}

	if (hInternet) InternetCloseHandle(hInternet);
	hInternet = InternetOpenW(settings.userAgent.cStr(),accessType,proxyName.cStr(),0,0);
	if (hInternet == 0) 
		throw FileMsgException(THISLOCATION,GetLastError(),url,"Cannot initialize WinInet");

	if (hConnect) InternetCloseHandle(hConnect);
	hConnect = InternetConnectW(hInternet,domain.cStr(),(INTERNET_PORT)port,
		username.empty()?0:username.cStr(),
		password.empty()?0:password.cStr(),
		INTERNET_SERVICE_HTTP ,0,0);
	if (hConnect == 0) 
		throw FileMsgException(THISLOCATION,GetLastError(),url,"Cannot connect remote site");
	
	DWORD reqFlags = INTERNET_FLAG_NO_UI |INTERNET_FLAG_HYPERLINK ;
	if (redirDisabled) reqFlags|=INTERNET_FLAG_NO_AUTO_REDIRECT ;
	if (!settings.cookiesEnabled) reqFlags|=INTERNET_FLAG_NO_COOKIES;
	if (secure) reqFlags|=INTERNET_FLAG_SECURE;
	

	hHTTPConn = HttpOpenRequestW(hConnect,String(method).cStr(),path.cStr(),
					0,0,0,reqFlags,0);
	if (hHTTPConn == 0) 
		throw FileMsgException(THISLOCATION,GetLastError(),url,"Cannot connect remote site");
	AutoArray<wchar_t> hdrall;
	for (HeaderMap::Iterator iter = hdrmap.getFwIter(); iter.hasItems();) {

		const HeaderMap::Entity &e = iter.getNext();
		fmt(L"%1: %2\n") << e.key << e.value;
		hdrall.append(fmt.write());
	}	
	if (!hdrall.empty() &&
		!HttpAddRequestHeadersW(hHTTPConn,hdrall.data(),(DWORD)hdrall.length(),HTTP_ADDREQ_FLAG_REPLACE|HTTP_ADDREQ_FLAG_ADD))
		throw FileMsgException(THISLOCATION,GetLastError(),url,"AddRequest failed");
	
	if (!HttpSendRequestW(hHTTPConn,0,0,postBuffer.data(),(DWORD)postBuffer.length())) {
		bool stillError = true;
		DWORD dwError = GetLastError();	
		if (dwError == ERROR_INTERNET_INVALID_CA && settings.allowUntrustedCert) {
			DWORD dwFlags;
			DWORD dwBuffLen = sizeof(dwFlags);

			InternetQueryOption (hHTTPConn, INTERNET_OPTION_SECURITY_FLAGS,
				(LPVOID)&dwFlags, &dwBuffLen);

			dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
			InternetSetOption (hHTTPConn, INTERNET_OPTION_SECURITY_FLAGS,
				&dwFlags, sizeof (dwFlags) );
			if (HttpSendRequestW(hHTTPConn,0,0,postBuffer.data(),(DWORD)postBuffer.length()))
				stillError = false;			
		}
		if (stillError)
			throw FileMsgException(THISLOCATION,GetLastError(),url,"Failed to SendRequest");
	}
	postBuffer.clear();
}

void WinHttpStream::sendRequest()
{
	if (!exceptionDisabled) {
		curState = stReadResponse;
		 natural status = getStatusCode();
		 if (status >= 300)
			 throw HttpStatusException(THISLOCATION,url,status,String());

	}
}

bool WinHttpStream::inConnected() const
{
	return curState != stIdle;
}

LightSpeed::ConstStrA WinHttpStream::getHeader( ConstStrA field )
{
	class FillHdrMap: public IEnumHeaders {
	public:
		HeaderMap &hdrmap;
		FillHdrMap(HeaderMap &hdrmap):hdrmap(hdrmap) {}
		virtual bool operator()(ConstStrA field, ConstStrA value) {
			hdrmap.insert(field, value);
			return false;
		}
	};
	if (curState != stReadResponse) {
		setState(stReadResponse);
		hdrmap.clear();
		FillHdrMap x(hdrmap);
		enumHeaders(x);
	}
	
	StringA *k = hdrmap.find(StringAI(field));
	if (k == 0) return ConstStrA();
	else return *k;
}

bool WinHttpStream::enumHeaders( IEnumHeaders &enumHdr )
{
	setState(stReadResponse);

	AutoArray<char> buffer;
	buffer.resize(1000);
	DWORD size = (DWORD)buffer.length();
	while (!HttpQueryInfoA(hHTTPConn,HTTP_QUERY_RAW_HEADERS,
		buffer.data(),&size,0)) {
			DWORD res = GetLastError();
			if (res == ERROR_INSUFFICIENT_BUFFER) {
				buffer.resize(size);
			} else {
				throw ErrNoWithDescException(THISLOCATION,GetLastError(),
					"Cannot retrieve headers from the request");
			}
	}
	buffer.resize(size);

	AutoArray<char>::SplitIterator iter=buffer.split((char)0);
	TextParser<char> parser;
	while (iter.hasItems()) {
		ConstStrA line = iter.getNext();
		if (parser("%1 : %2",line)) {
			ConstStrA field = parser[1].str();
			ConstStrA value = parser[2].str();
			if(enumHdr(field, value)) return true;
		}
	}

	return false;


}

void WinHttpStream::closeOutput()
{
	setState(stReadResponse);	
}


}
