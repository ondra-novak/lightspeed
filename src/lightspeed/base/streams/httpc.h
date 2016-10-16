#pragma once

#include "../containers/constStr.h"
#include "../containers/stringpool.h"
#include "../containers/map.h"
#include "../containers/autoArray.h"
#include "fileio.h"

namespace LightSpeed {



	template<typename FnOutput>
	class HttpRequest: public WriteIteratorBase<byte, HttpRequest<FnOutput> > {
	public:

		HttpRequest(const FnOutput &output);
		~HttpRequest();
		HttpRequest(const HttpRequest &other);

		void reset();
		void setMethod(ConstStrA method);
		void setPath(ConstStrA path);
		///Setups path and headers for specified url
		/**
		@param url the url which is used to setup headers
		@param proxy if true, function creates request for proxy

		@note Function sets following items
		  - path
		  - Host
		  - Authorization if username and password supplied
		*/
		bool setUrl(ConstStrA url, bool proxy = false);
		///Adds header
		/**
		 @param hdr header's field name
		 @param value header's value
		 @note Following headers cannot be set
		   - Transfer-Encoding - because this client always uses "chunked"
		       If you need compression, use Content-Encoding
		   - Content-Length - in chunked encoding is not required
		  
	    */
		void addHeader(ConstStrA hdr, ConstStrA value);

		void send(ConstBin data);
		void send();
		void beginBody();


		void sendHeaders(bool body = false);
		bool hasItems() const;
		void write(const byte &b);
		void operator()(byte b);
		void flush();

		void setMaxChunkSize(natural s);
		

	protected:

		typedef StringPoolA::Str StrP;
		typedef Map<StrP, StrP> HdrMap;

		StringPoolA strPool;
		StrP method;
		StrP path;
		HdrMap hdrMap;
		bool headersSent;
		AutoArray<byte> chunkBuff;
		natural maxChunkSize;

		FnOutput output;

		void sendString(ConstStrA str);
		void sendString(ConstBin str);
		void sendChunk();
	};

	///
	/**
		@tparam FnInput function that reads input

		@code
		int inputFn()
		@endcode

		function returns number between 0 - 255 as byte, or -1 as eof

	*/
	template<typename FnInput>
	class HttpResponse: public IteratorBase<byte, HttpResponse<FnInput> > {
	public:
		HttpResponse(const FnInput &input);
		
		///Opens response
		/**
		Function expects http response. It starts to read
		response line and headers and stops on begin of the body.
		It also select correct transfer encoding. The
		class supports only identity and chunked. 

		@return status code of response. Returns naturalNull
		if the function cannot parse the header

		Function is called automatically when one of following
		function is called: hasItems(), getNext(), peek(), getRemain()
		*/
		natural open();

		///Resets current response state
		/**
			The function reset also skips any unread body. 

			@retval true request has been reset. 
					It can be opened again.
			@retval false the request cannot be reset. This
					connection should be closed. This can
					happen, when body has no length, so
					only way to reset the response is to
					close connection
		*/
		bool reset();
		///Deternines whether header is defined
		/**
		@param name header name
		@retval true header is defined
		@retval false header is not defined
		*/
		bool headerDefined(ConstStrA name) const;

		///Retrieves header's value
		/**
			@param name header's name
			@return header's value. If header is not
			defined, returns empty string
		*/
		ConstStrA getHeader(ConstStrA name) const;

		///Retrieves status code
		natural getStatusCode() const;

		///Retrieves status message
		ConstStrA getStatusMessage() const;

		bool hasItems() const;
		const byte &getNext();
		const byte &peek() const;

		///Retrieves length of the body 
		/**
		Function returns remain count of bytes if called while reading

		@retval naturalNull unknown length. The transfer is chunked.
		@retval number length in bytes.
		@retval 0 no more bytes, eof reached (empty chunk reached)
		*/
		natural getRemain() const;

	protected:
		typedef StringPoolA::Str StrP;
		typedef Map<StrP, StrP> HdrMap;

		mutable StringPoolA strPool;
		StrP statusMessage;
		StrP httpVersion;
		natural statusCode;
		HdrMap hdrMap;
		bool reqOpened;
		bool headerParsed;
		bool chunkedRead;		
		mutable bool eof;
		mutable natural readLen;

		mutable FnInput input;
		mutable byte c;
		mutable bool cloaded;

		enum ReadMode {
			rmDirect,
			rmLimited,
			rmChunked
		};

		ReadMode readMode;


		static const byte eofByte = 0xFF;
		StrP readLine() const;
		static natural parseNumber(ConstStrA n, natural base);
		static void trim(ConstStrA &str);
		bool openChunk() const;
		bool fetchByte() const;
	};

	class HttpClientInstance {
	protected:
		class Reader {
		public:
			Reader(SeqFileInput in) :in(in) {}
			int operator()() {
				if (in.hasItems()) return in.getNext();
				else return -1;
			}
		protected:
			SeqFileInput in;
		};

		class Writer {
		public:
			Writer(SeqFileOutput out) :out(out) {}
			void operator()(ConstBin data) {
				out.blockWrite(data, true);
			}
		protected:
			SeqFileOutput out;
		};

	public:

		typedef HttpRequest<Writer> Request;
		typedef HttpResponse<Reader> Response;

		Request request;
		Response response;

		HttpClientInstance(PInOutStream bufferedStream)
			:request(Writer(SeqFileOutput(bufferedStream)))
			, response(Reader(SeqFileInput(bufferedStream))) {}
			



	protected:
	
	};

}
