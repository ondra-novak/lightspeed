#pragma once
#include "ioexception.h"

namespace LightSpeed {

	class HttpStatusException: public IOException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL

		HttpStatusException(const ProgramLocation &loc, 
			const String &url, natural status, const String &statusMsg):
			Exception(loc),IOException(loc),
			url(url),status(status),statusMsg(statusMsg) {}

		const String &getUrl() const {return url;}
		const String &getStatusMsg() const {return statusMsg;}
		natural getStatus() const {return status;}

		static LIGHTSPEED_EXPORT const char *msgText;;
		~HttpStatusException() throw() {}

	protected:
		String url;
		natural status;
		String statusMsg;

		void message(ExceptionMsg &msg) const {
			msg(msgText) << url << status << statusMsg;
		}
	};

}
