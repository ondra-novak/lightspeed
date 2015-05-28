#include "../containers/string.h"
#include "../streams/fileio_ifc.h"
#include "../containers/stringpool.h"
#include "../../mt/process.h"
#include "../streams/netio_ifc.h"
#pragma once


namespace LightSpeed {

	struct HTTPSettings {
		IHTTPSettings::ProxyMode proxyMode;
		bool cookiesEnabled;
		bool allowUntrustedCert; //allow untrusted certificates

		String proxyAddr;
		natural proxyPort;
		String userAgent;
		natural defaultTimeout;
		natural ipver;

		HTTPSettings():proxyMode(IHTTPSettings::pmAuto)
			,cookiesEnabled(false),allowUntrustedCert(false),defaultTimeout(naturalNull),ipver(0) {}
	};

    class LinuxHTTPSettings: public IHTTPSettings {
    public:

    	LinuxHTTPSettings(HTTPSettings &settings):settings(settings) {}

		virtual void setUserAgent(const String &uagent) {settings.userAgent = uagent;}
		virtual String getUserAgent() const  {return settings.userAgent;}
		virtual void setProxy(ProxyMode md, String addr, natural port) {
			settings.proxyMode = md;
			settings.proxyAddr = addr;
			settings.proxyPort = port;
		}
		virtual ProxyMode getProxy() const  {return settings.proxyMode;}
		virtual void getProxySettings(String &addr, natural &port) const {
			addr = settings.proxyAddr;
			port = settings.proxyPort;
		}
		///Enables cookies
		/**
		 This allows to object save and load cookies internally.
		 Implementation strongly depends on platform, but can
		 be useful, if you want to make multiple requests while
		 cookies are kept between requests without need to implement
		 fully functional cookie store.

		 Under Microsoft Windows, function uses global cookie
		 repository, which is shared by all applications in
		 context of current user. Session cookies are valid until
		 processes exists.

		 Under Linux, cookies are stored into temporary file, which
		 is deleted on process exit.

		 @param enable true to enable cookies, false to disable cookies
		 */
		virtual void enableCookies(bool enable) {
			settings.cookiesEnabled = enable;
		}
		virtual bool areCookiesEnabled() const {return settings.cookiesEnabled;}
		virtual void setIOTimeout(natural timeout) {settings.defaultTimeout = timeout;}
		virtual natural getIOTimeout() const {return settings.defaultTimeout;}
		virtual void forceIPVersion(natural ver)  {settings.ipver = ver;}
		virtual void allowUntrustedCerts( bool allow ) {settings.allowUntrustedCert = allow;}

    protected:
		HTTPSettings &settings;
    };



	class LinuxHttpStream: public IHTTPStream, public LinuxHTTPSettings, public IFileExtractHandle {
	public:

		LinuxHttpStream(String url, const HTTPSettings &defaultSettins);

		~LinuxHttpStream();

		///Reads bytes into buffer
        /** @param buffer pointer to buffer that will receive binary data
         *  @param size size of buffer in bytes
         *  @return count of bytes read. In case of blocking I/O
         *  function must be able to read atleast one byte. Otherwise, it
         *  can block operation. In case of non-blocking I/O, function
         *  can return zero to report, that operation would block,
         *  @exception IOException thrown on any I/O error
         */
        virtual natural read(void *buffer,  natural size);
        /// Writes bytes from buffer to file
        /** @param buffer pointer to buffer that contains data to write
         *  @param size size of buffer in bytes
         *  @return count of bytes written. In case of blocking I/O
         *  function must be able to write atleast one byte. Otherwise, it
         *  can block operation. In case of non-blocking I/O, function
         *  can return zero to report, that operation would block
         *  @exception IOException thrown on any I/O error
         */
        virtual natural write(const void *buffer,  natural size);


		///Reads data from the stream, but did not remove them
		/**
		 * @param buffer pointer to buffer, which receives data
		 * @param size size of buffer
		 * @return count of bytes written to the buffer.
		 *
		 * @note function should store at least one byte. Stream don't need to
		 * support function for more than one byte. If there is no bytes
		 * in internal buffer, function starts waiting to receive at least
		 * one byte.
		  */
		virtual natural peek(void *buffer, natural size) const;


		///Returns true, when stream can be read
		/**
		 * @retval true can read the stream
		 * @retval false cannot read stream. stream is not opened for reading, or EOF reached
		 */

		virtual bool canRead() const;

		///Returns true, when stream can be written
		/**
		 * @retval true you can write
		 * @retval false you cannot write
		 */

		virtual bool canWrite() const;


		///Flushes any internal buffer
		virtual void flush();

		virtual natural dataReady() const;




		///Allows to change request method
		/**
		 * Function must be called before first data are read or sent
		 * 
		 * @param method text string contains HTTP method. Default value
		 *   id "GET". You can change it to "HEAD", "POST", "PUT" and other
		 *   methods. Methods that expects body in the request requires to define
		 *  Content-Length: in header and the body is sent using write
		 *  write() called before read().
		 *
		 * @note Not all implementation supports all methods. Function
		 * is useful to change default GET to POST. To perform advanced
		 * task in HTTP protocol you need special library
		 */
		 
		virtual IHTTPStream &setMethod(ConstStrA method);

		///Sets additional header
		/**
		 * @param headerName name of header
		 * @param headerValue content of header
		 *
		 * @note existing headers are replaced
		 */
		virtual IHTTPStream &setHeader(ConstStrA headerName, ConstStrA headerValue);


		///Cancels any pending waiting operation
		/**
		 * Function should be called from different thread. 
		 *                                                  
		 * After canceling request, state of object is undefined. You 
		 * cannot receive reply headers, and stream can be uncompleted.
		 * Some implementation may cause throwing exception from read()
		 * function.
		 */
		virtual IHTTPStream &cancel();

		///Retrieves reply headers
		/** Headers are available after first byte is sent, or
		 * after connect() is returned. If request is
		 * needs body, headers are available after body is sent
		 */
		virtual StringA getReplyHeaders();


		///Retrieves header from the reply
		/**
		 * @param field case insensitive field name
		 * @return content of the header
		 *
		 * Asking for headers causes to send request and flush output stream.
		 * Function then waits for the reply.
		 */

		virtual ConstStrA getHeader(ConstStrA field);



		virtual bool enumHeaders(IEnumHeaders &enumHdr);

		///Retrieves whether object is connected
		/** @retval true object is connected and you can download data.
		 * This state is also reported, after connection of POST request while
		 * sending request body */
		virtual bool inConnected() const;
	
		///Disables automatic redirects
		/**
		 * By default, any redirect is interpreted. In this case
		 * you never get redirect HTTP code as reply. Call this function
		 * to disable automatic redirect for this request. 
		 *
		 * Function must be called before request is sent, otherwise it
		 * has no effect
		 */
		 
		virtual IHTTPStream &disableRedirect();

		///Disables status exceptions
		/** 
		 * By default, any status different to 200 (and different to redirects,
		 * when automatic redirections are not disabled) are thrown as 
		 * exception. Calling this method causes, that exception is not thrown
		 * and you can read status code using method getStatusCode(). 
		 *
		 * Default settings allows to simulate disk-stream when consumer cannot
		 * manually check status code and does't access this interface
		 */
		virtual IHTTPStream &disableException();

		///Connects the remote server by address
		/** 
		 * By default, connection made when first I/O is requested. You
		 * can connect manually using this function causing send request
		 * If method requtires body of request, you should send whole
		 * body after the request */
		virtual IHTTPStream &connect();

		///Retrieves status code
		/**
		 * @return status code of operation. Status code is available
		 * after request is sent including the body
		 *
		 */
		virtual natural getStatusCode();

		virtual void setUserAgent(const String &uagent) {return LinuxHTTPSettings::setUserAgent(uagent);}
		virtual String getUserAgent() const {return LinuxHTTPSettings::getUserAgent();}
		virtual void setProxy(ProxyMode md, String addr, natural port) {return LinuxHTTPSettings::setProxy(md,addr,port);}
		virtual ProxyMode getProxy() const {return LinuxHTTPSettings::getProxy();}
		virtual void getProxySettings(String &addr, natural &port) const {return LinuxHTTPSettings::getProxySettings(addr,port);}
		virtual void enableCookies(bool enable) {return LinuxHTTPSettings::enableCookies(enable);}
		virtual bool areCookiesEnabled() const {return LinuxHTTPSettings::areCookiesEnabled();}
		virtual natural getIOTimeout() const {return LinuxHTTPSettings::getIOTimeout();}
		virtual void setIOTimeout(natural timeout) {return LinuxHTTPSettings::setIOTimeout(timeout);}
		virtual void forceIPVersion(natural ver)  {LinuxHTTPSettings::forceIPVersion(ver);}
		virtual size_t getHandle(void *buffer, size_t bufferSize);
		virtual void allowUntrustedCerts( bool allow ) {WinHTTPSettings::allowUntrustedCerts(allow);}


		///Closes the output and send request to the other side
		/**
		 * For http stream, this function forces to send request to the
		 * other side so you can read response using read() or readAll()
		 *
		 * If request is already sent, function does nothing
		 */
		virtual void closeOutput();

	protected:
		enum State{
			stIdle,
			stWriting,
			stReading
		};

		String url;
		HTTPSettings settings;
		State state;
		mutable SeqFileInput inputStream;
		SeqFileOutput outputStream;
		StringA method;
		typedef Map<StringPoolStrRef<char>,StringPoolStrRef<char>, TestCompare<StringPoolStrRef<char>,StrCmpCI<char>, cmpResultLess> > Header;
		typedef Map<ConstStrA,ConstStrA, TestCompare<ConstStrA,StrCmpCI<char>, cmpResultLess> > ReplyHeader;
		StringPool<char> headerPool;
		AutoArray<char> replyHdrPool;
		Header sendHeader;
		ReplyHeader replyHeader;
		Process wgetProc;
		PTemporaryFile tempBuffer;
		natural statusCode;
		ConstStrA statusMsg;
		bool noredir;
		bool noexecpt;

		void sendRequest();
		void preparePost();

private:
	static void cropLine(ConstStrA& hdrline);
void parseReplyHeader();
void readHeader(SeqFileInput& wgetin, SeqFileInput& wgeterr);

	};

	class LinuxHttpHandler: public IFileIOHandler {
	public:
        virtual PInOutStream openSeqFile(ConstStrW fname,  FileOpenMode mode, OpenFlags::Type flags);
        virtual PRndFileHandle openRndFile(ConstStrW ofn, FileOpenMode mode, OpenFlags::Type flags);
        virtual bool canOpenFile(ConstStrW name, FileOpenMode mode) const;
        virtual PFolderIterator openFolder(ConstStrW pathname);
		virtual PFolderIterator getFileInfo(ConstStrW pathname);
        virtual void createFolder(ConstStrW folderName, bool parents);
        virtual void removeFolder(ConstStrW folderName, bool recursive);
        virtual void copy(ConstStrW from, ConstStrW to, bool overwrite);
        virtual void move(ConstStrW from, ConstStrW to, bool overwrite);
        virtual void link(ConstStrW linkName, ConstStrW target, bool overwrite);
        virtual void remove(ConstStrW what);
        virtual PMappedFile mapFile(ConstStrW filename, FileOpenMode mode);

	protected:
        HTTPSettings httpSettings;


	};



}
