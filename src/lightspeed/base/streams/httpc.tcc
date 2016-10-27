#pragma once

#include "../containers/stringpool.tcc"
#include "../containers/map.tcc"
#include "httpc.h"
#include "../text/toString.h"
#include "../iter/iterConvBytes.h"
#include "../../utils/base64.h"
#include "../containers/convertString.h"
#include "../containers/autoArray.tcc"

namespace LightSpeed {



	template<typename FnOutput>
	inline HttpRequest<FnOutput>::HttpRequest(const FnOutput & output)
		:headersSent(false),maxChunkSize(65536),output(output)
	{
	}

	template<typename FnOutput>
	HttpRequest<FnOutput>::~HttpRequest()
	{
		reset();
	}
	template<typename FnOutput>
	HttpRequest<FnOutput>::HttpRequest(const HttpRequest &other)
		:headersSent(false), maxChunkSize(other.maxChunkSize), output(other.output) {}


	template<typename FnOutput>
	void HttpRequest<FnOutput>::reset()
	{
		if (headersSent) {
			flush();
			sendChunk();
		}
		headersSent = false;
		hdrMap.clear();
		method.clear();
		path.clear();
		strPool.clear();
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::setMethod(ConstStrA method)
	{
		this->method = strPool.add(method);
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::setPath(ConstStrA path)
	{
		this->path = strPool.add(path);
	}

	template<typename FnOutput>
	bool HttpRequest<FnOutput>::setUrl(ConstStrA url, bool proxy)
	{
		

		natural protoSep = url.find(ConstStrA("://"));
		if (protoSep == naturalNull) return false;
		ConstStrA proto = url.head(protoSep);
		if (proto != "http" && proto != "https" && proto != "ws" && proto != "wss") {
			return false;
		}
		
		if (proxy) {
			setPath(url);
			url = url.offset(protoSep + 3);
			natural pathSep = url.find('/');
			if (pathSep != naturalNull) {
				url = url.head(pathSep);
			}
		}
		else {
			url = url.offset(protoSep + 3);
			natural pathSep = url.find('/');
			if (pathSep == naturalNull) {
				setPath("/");
			}
			else {
				setPath(url.offset(pathSep));
				url = url.head(pathSep);
			}
		}
		natural authSep = url.find('@');
		if (authSep != naturalNull) {
			ConstStrA auth = url.head(authSep);
			CharsToBytesConvert charToByte;
			ByteToBase64Convert base64conv;
			ConverterChain<CharsToBytesConvert &, ByteToBase64Convert &> convChain(charToByte, base64conv);

			StringA authStr = ConstStrA("Basic ")
				+ StringA(convertString(convChain, auth));
			addHeader("Authorization", authStr);
			url = url.offset(authSep + 1);
		}
		addHeader("Host", url);
		return true;
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::addHeader(ConstStrA hdr, ConstStrA value)
	{
		hdrMap.replace(strPool.add(hdr), strPool.add(value));
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::send(ConstBin data)
	{
		addHeader("Content-Length", ToString<natural>(data.length()));
		sendHeaders(false);
		sendString(data);
		headersSent = false;
		reset();
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::send()
	{
		sendHeaders(false);
		headersSent = false;
		reset();
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::beginBody()
	{
		sendHeaders(true);
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::sendHeaders(bool body)
	{
		if (body) {
			addHeader("Transfer-Encoding", "chunked");
			hdrMap.erase(StrP(ConstStrA("Content-Length	")));
		}

		sendString(method);
		sendString(" ");
		sendString(path);
		sendString(" HTTP/1.1\r\n");

		for (HdrMap::Iterator iter = hdrMap.getFwIter(); iter.hasItems();) {
			const HdrMap::KeyValue &kv = iter.getNext();
			sendString(kv.key);
			sendString(": ");
			sendString(kv.value);
			sendString("\r\n");
		}
		sendString("\r\n");
		headersSent = true;
	}

	template<typename FnOutput>
	bool HttpRequest<FnOutput>::hasItems() const
	{
		return true;
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::write(const byte & b)
	{
		chunkBuff.add(b);
		if (chunkBuff.length() == maxChunkSize) flush();
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::operator()(byte b)
	{
		write(b);
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::flush()
	{
		if (!headersSent) sendHeaders();
		if (!chunkBuff.empty()) {
			sendChunk();
			chunkBuff.clear();
		}
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::setMaxChunkSize(natural s)
	{
		maxChunkSize = s;
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::sendString(ConstStrA str)
	{
		output(ConstBin((const byte *)str.data(), str.length()));
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::sendString(ConstBin str)
	{
		output(str);
	}

	template<typename FnOutput>
	void HttpRequest<FnOutput>::sendChunk()
	{
		sendString(ToString<natural>(chunkBuff.length(), 16));
		sendString("\r\n");
		sendString(chunkBuff);
		sendString("\r\n");
	}

//-------------------------------------------------------


	template<typename FnInput>
	inline HttpResponse<FnInput>::HttpResponse(const FnInput & input)
		:reqOpened(false)
		, headerParsed(false)
		, chunkedRead(false)
		, eof(true)
		, readLen(0)
		, input(input)
		, cloaded(false)
	{
	}
	
	template<typename FnInput>
	bool HttpResponse<FnInput>::fetchByte() const {
		int i = input();
		if (i == -1) {
			eof = true;
			return false;
		}
		c = (byte)i;
		cloaded = true;
		return true;
	}


	template<typename FnInput>
	natural HttpResponse<FnInput>::open()
	{
		if (reqOpened) return statusCode;
		reqOpened = true;
		headerParsed = false;
		eof = false;

		StrP hdrline = readLine();
		while (hdrline.empty()) {
			strPool.clear();
			hdrline = readLine();
		}
		StrP::SplitIterator iter = hdrline.split(' ');
		httpVersion = strPool.add(ConstStrA(iter.getNext()));
		ConstStrA statusCodeStr = strPool.add(ConstStrA(iter.getNext()));
		statusMessage = strPool.add(ConstStrA(iter.getRest()));
		statusCode = parseNumber(statusCodeStr,10);

		hdrline = readLine();

		while (!hdrline.empty()) {
			ConstStrA hdrl = hdrline;
			natural sep = hdrline.find(':');
			if (sep == naturalNull) { return naturalNull; }
			ConstStrA field = hdrl.head(sep);
			ConstStrA value = hdrl.offset(sep + 1);
			trim(field);
			trim(value);
			hdrMap.insert(strPool.add(field),strPool.add(value));
			hdrline = readLine();
		}

		ConstStrA te = getHeader("Transfer-Encoding");
		if (te == "chunked") {
			chunkedRead = true;
			readLen = 0;
			readMode = rmChunked;
		}
		else {
			chunkedRead = false;
			ConstStrA cl = getHeader("Content-Length");
			if (!cl.empty()) {
				readLen = parseNumber(cl,10);
				readMode = rmLimited;
			}
			else {
				readLen = naturalNull;
				readMode = rmDirect;
			}
		}
		cloaded = false;
		headerParsed = true;
		return statusCode;
	}
	

	template<typename FnInput>
	bool HttpResponse<FnInput>::reset()
	{
		if (reqOpened) {
			if (headerParsed) {
				if (readLen != naturalNull) {
					while (hasItems()) this->skip();
				}
				else {
					return false;
				}
			}
			else {
				return false;
			}
		}
		httpVersion.clear();
		statusCode = 0;
		statusMessage.clear();
		hdrMap.clear();
		strPool.clear();
		reqOpened = false;
		headerParsed = false;
		eof = false;
		return true;
		
	}

	template<typename FnInput>
	bool HttpResponse<FnInput>::headerDefined(ConstStrA name) const
	{
		return hdrMap.find(name) != 0;
	}

	template<typename FnInput>
	ConstStrA HttpResponse<FnInput>::getHeader(ConstStrA name) const
	{
		const StrP *fnd = hdrMap.find(name);
		if (fnd == 0)
			return ConstStrA();
		else
			return *fnd;
	}

	template<typename FnInput>
	natural HttpResponse<FnInput>::getStatusCode() const
	{
		return statusCode;
	}

	template<typename FnInput>
	ConstStrA HttpResponse<FnInput>::getStatusMessage() const
	{
		return statusMessage;
	}

	template<typename FnInput>
	bool HttpResponse<FnInput>::hasItems() const
	{			
		switch (readMode) {
		case rmDirect: 
			if (cloaded) return true;
			return fetchByte();
		case rmLimited:			
			return cloaded || readLen > 0;
		case rmChunked:
			if (cloaded || readLen > 0) return true;
			return openChunk();
		default:
			return false;
		}
	}


	template<typename FnInput>
	bool HttpResponse<FnInput>::openChunk() const
	{
		natural p = strPool.mark();
		ConstStrA ln = readLine();
		while (ln.empty()) {
			ln = readLine();
		}
		readLen = parseNumber(ln, 16);
		if (readLen == 0) {
			readLine();
			eof = true;
			return false;
		}
		else {
			cloaded = false;
			strPool.release(p);
			return true;
		}
	}

	template<typename FnInput>
	const byte & HttpResponse<FnInput>::getNext() 
	{
		switch (readMode) {
		case rmDirect:
			if (!cloaded) {
				if (!fetchByte()) throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
			}
			cloaded = false;
			return c;
		case rmLimited:
			if (!cloaded) {
				if (!fetchByte()) throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
			}
			readLen--;
			cloaded = false;
			return c;
		case rmChunked:
			if (!cloaded) {
				if (readLen == 0) {
					if (!openChunk())
						throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
				}
				if (!fetchByte()) throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
			}
			readLen--;
			cloaded = false;
			return c;
		}
		throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
		return c;
	}

	template<typename FnInput>
	const byte & HttpResponse<FnInput>::peek() const
	{
		switch (readMode) {
		case rmDirect:
			if (!cloaded) {
				if (!fetchByte()) throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
			}
			return c;
		case rmLimited:
			if (!cloaded) {
				if (!fetchByte()) throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
			}
			return c;
		case rmChunked:
			if (!cloaded) {
				if (readLen == 0) {
					openChunk();
					if (eof)
						throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
				}
				if (!fetchByte()) throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
			}
			return c;
		}
		throwIteratorNoMoreItems(THISLOCATION, typeid(*this));
		return c;
	}

	template<typename FnInput>
	natural HttpResponse<FnInput>::getRemain() const
	{
		switch (readMode) {
		case rmDirect: return hasItems() ? naturalNull : 0;
		case rmLimited: return hasItems() ? readLen : 0;
		case rmChunked: return hasItems() ? naturalNull : 0;
		default: return 0;
		}
	}


	template<typename FnInput>
	typename HttpResponse<FnInput>::StrP HttpResponse<FnInput>::readLine() const
	{
		byte prev = 0;
		StringPoolA::WriteIterator iter = strPool.getWriteIterator();
		while (!eof) {
			int z = input();
			if (z == -1) {
				eof = true;				
			}
			else {
				iter.write((byte)z);
				if (prev == '\r' && z == '\n') {
					ConstStrA res = iter.finish();
					return strPool.add(res.crop(0, 2));
				}
				else {
					prev = (byte)z;
				}
			}
		}
		return ConstStrA();
	}

	template<typename FnInput>
	natural HttpResponse<FnInput>::parseNumber(ConstStrA nstr, natural base)
	{
		natural n = 0;
		ConstStrA::Iterator iter = nstr.getFwIter();
		while (iter.hasItems()) {
			byte c = iter.getNext();
			if (c >= '0' && c<='9') {
				n = c - '0';
				break;
			}
			else if (c >= 'A' && c < ('A' + base - 10)) {
				n = c - 'A' + 10;
				break;
			}
			else if (c >= 'a' && c < ('a' + base - 10)) {
				n = c - 'a' + 10;
				break;
			}

			else if (c != ' ' && c != '\t') {
				return 0;
			}
		}

		while (iter.hasItems()) {
			byte c = iter.getNext();
			if (c >= '0' && c <= '9') {
				n = n * base + c - '0';
			}
			else if (c >= 'A' && c < ('A' + base - 10)) {
				n = n * base + c - 'A' + 10;
			}
			else if (c >= 'a' && c < ('a' + base - 10)) {
				n = n * base + c - 'a' + 10;
				break;
			}
			else {
				break;
			}
		}
		return n;
	}

	template<typename FnInput>
	void HttpResponse<FnInput>::trim(ConstStrA & str)
	{
		while (!str.empty() && (str[0] == ' ' || str[0] == '\t'))
			str = str.crop(1, 0);
		while (!str.empty() && (str.tail(1)[0] == ' ' || str.tail(1)[0] == '\t'))
			str = str.crop(0, 1);
	}

}
