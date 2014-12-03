/** @file
 * all basic network exceptions
 */

#include "systemException.h"
#pragma once

namespace LightSpeed {

	///Exception class common to all network exceptions
	class NetworkException: public virtual Exception {
	public:

		NetworkException(const ProgramLocation &loc) {
			Exception::setLocation(loc);
		}
		virtual ~NetworkException() throw () {}
	};

	///General network I/O error
	/** This exception is thrown every time when unexpected exception is
	    happen. Exception object carries error code, that is specific
		for the current platform */
	class NetworkIOError: public NetworkException, public ErrNoWithDescException {
	public:

		LIGHTSPEED_EXCEPTIONFINAL;

		NetworkIOError(const ProgramLocation &loc, int error_nr,
			const String &desc)
			:NetworkException(loc),ErrNoWithDescException(loc,error_nr,desc) {}

		static LIGHTSPEED_EXPORT const char *msgText;;
	protected:
		void message(ExceptionMsg &msg) const {
			msg(msgText);
			ErrNoWithDescException::message(msg);
		}
		const char *getStaticDesc() const {
			return "General network I/O error. Note that error code can be "
				"platform specific";
		}
	};

	///thrown, when request for listening connection is opened on invalid or busy port number
	class NetworkPortOpenException: public NetworkException, public ErrNoException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		NetworkPortOpenException(const ProgramLocation &loc, int error_nr,
			natural port)
			:NetworkException(loc),ErrNoException(loc,error_nr),port(port) {}
	
		///Retrieves port number
		natural getPort() const {return port;}

		static LIGHTSPEED_EXPORT const char *msgText;;

	protected:
		natural port;

		void message(ExceptionMsg &msg) const {
			msg(msgText) << port;
			ErrNoException::message(msg);
		}
		const char *getStaticDesc() const {
			return "Exception is thrown, when server application is trying "
				"to open local port, while port is already used. It can be "
				"also another problem for example resource leaking, or "
				"resource exhausting";
		}
	};

	///thrown, when invalid address is used for connection
	class NetworkInvalidAddressException: public NetworkException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		NetworkInvalidAddressException(const ProgramLocation &loc,
			PNetworkAddress address)
			:NetworkException(loc),address(address) {}
		///Retrieves address name
		PNetworkAddress getAddress() const {return address;}

		virtual ~NetworkInvalidAddressException() throw () {}

		static LIGHTSPEED_EXPORT const char *msgText;;
		static LIGHTSPEED_EXPORT const char *msgTextNull;

	public:
		PNetworkAddress address;

		void message(ExceptionMsg &msg) const {
			if (address == nil) msg(msgText) << msgTextNull;
			else msg(msgText) << address->asString(false);
		}
	};

	///Timeout in network I/O
	class NetworkTimeoutException: public NetworkException {
	public:
		enum Operation {
			///Operation is not specified
			unspecified,
			///Time elapsed while reading from the network
			reading,
			///Time elapsed while writing to the network
			writing,
			///Connection has not been established in the time
			connecting,
			///No connection in-came during waiting
			listening,
		};

		LIGHTSPEED_EXCEPTIONFINAL;
		NetworkTimeoutException(const ProgramLocation &loc, 
			natural timeout, Operation oper)
			:NetworkException(loc),timeout(timeout),oper(oper) {}
		natural getTimeout() const {return timeout;}
		Operation getOperation() const {return oper;}

		static LIGHTSPEED_EXPORT const char *msgText;;
		static LIGHTSPEED_EXPORT const char *msgTextReading;
		static LIGHTSPEED_EXPORT const char *msgTextWriting;
		static LIGHTSPEED_EXPORT const char *msgTextConnecting;
		static LIGHTSPEED_EXPORT const char *msgTextListening;
		static LIGHTSPEED_EXPORT const char *msgTextUnknown;

	protected:
		natural timeout;
		Operation oper;

		void message(ExceptionMsg &msg) const {
			const char *x;
			switch(oper) {
			case reading: x = msgTextReading;break;
			case writing: x = msgTextWriting;break;
			case connecting: x = msgTextConnecting;break;
			case listening: x = msgTextListening;break;
			default: x = msgTextUnknown;break;
			}

			msg(msgText) << x << timeout;

		}
	};

	///Thrown, when system is unable to make remote connection
	class NetworkConnectFailedException: public NetworkException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		NetworkConnectFailedException(const ProgramLocation &loc, PNetworkAddress address)
			:NetworkException(loc),address(address) {}
		virtual ~NetworkConnectFailedException() throw () {}

		PNetworkAddress getAddress() const {return address;}

		static LIGHTSPEED_EXPORT const char *msgText;;

	protected:
		PNetworkAddress address;

		void message(ExceptionMsg &msg) const {
			if (address == nil) msg(msgText) << NetworkInvalidAddressException::msgTextNull;
			else msg(msgText) << address->asString(false);
		}
	};

	class NetworkResolveError: public NetworkException, 
								public ErrNoWithDescException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		NetworkResolveError(const ProgramLocation &loc,int error_nr, 
			const String &address)
			:NetworkException(loc),ErrNoWithDescException(loc,error_nr,address)
		{}

		const String &getAddress() const {
			return ErrNoWithDescException::getDesc();
		}

		static LIGHTSPEED_EXPORT const char *msgText;;

	protected:
		void message(ExceptionMsg &msg) const {
			msg(msgText) << getAddress();
			ErrNoException::message(msg);
		}
		const char *getStaticDesc() const {
			return "Given address cannot be resolved";
		}
	};

}
