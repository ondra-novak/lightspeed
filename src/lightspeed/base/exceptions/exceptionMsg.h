/*
 * exceptionMsg.h
 *
 *  Created on: 24.12.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_EXCEPTIONMSG_H_
#define LIGHTSPEED_EXCEPTIONMSG_H_

#pragma once

#include "../types.h"
#include "../memory/sharedResource.h"
#include "../text/textFormatManip.h"

namespace LightSpeed {

	template<typename T> class ConstStringT;
	class ExceptionMsg;

	namespace _intr {
		class ExceptionMsgBase {
		public:
			virtual ExceptionMsg &arg(natural num)= 0;
			virtual ExceptionMsg &arg(integer num)= 0;
		};

		class ExceptionMsgBase2:public ExceptionMsgBase {
		public:
			using ExceptionMsgBase::arg;
			virtual ExceptionMsg &arg(unsigned int num)= 0;
			virtual ExceptionMsg &arg(int num)= 0;
		};


		class ExceptionMsgSink;

	}

	class INumberFormatter;
	class ExceptionMsg: public _intr::ExceptionMsgBase2 {
	public:


		virtual _intr::ExceptionMsgSink operator()(ConstStringT<char> pattern) = 0;
		virtual _intr::ExceptionMsgSink operator()(const char *pattern) = 0;
		virtual _intr::ExceptionMsgSink operator()(ConstStringT<wchar_t> pattern) = 0;
		virtual _intr::ExceptionMsgSink operator()(const wchar_t *pattern) = 0;
		virtual ExceptionMsg &setBase(natural b)= 0;
		virtual ExceptionMsg &setPrecision(natural p)= 0;
		virtual ExceptionMsg &setSci(natural p)= 0;
		void resetArgs();
		virtual ~ExceptionMsg() {}

		virtual ExceptionMsg &setNumberFormat(INumberFormatter *fmt)= 0;

	protected:
		friend class _intr::ExceptionMsgSink;

		using ExceptionMsgBase2::arg;
		virtual ExceptionMsg &arg(ConstStringT<char> text)= 0;
		virtual ExceptionMsg &arg(const char *text)= 0;
		virtual ExceptionMsg &arg(char chr)= 0;
		virtual ExceptionMsg &arg(ConstStringT<wchar_t> text)= 0;
		virtual ExceptionMsg &arg(const wchar_t *text)= 0;
		virtual ExceptionMsg &arg(wchar_t chr)= 0;
		virtual ExceptionMsg &arg(lnatural num)= 0;
		virtual ExceptionMsg &arg(linteger num)= 0;
		virtual ExceptionMsg &arg(float num)= 0;
		virtual ExceptionMsg &arg(double num)= 0;
		ExceptionMsg &arg(const TextFormatManip &manip) {
			manip.doManip(*this);
			return *this;
		}

		virtual ExceptionMsg &flush() = 0;

	};

	template<typename H, typename T>
	struct VarArg;

	namespace _intr {
	class ExceptionMsgSink: public SharedResource {
	public:

		template<typename T>
		ExceptionMsgSink &operator <<(T a) {ref.arg(a); return *this;}

		template<typename V, typename H>
		ExceptionMsgSink &operator <<(const VarArg<V,H> &a) {
			operator << (a.next);
			return operator << (a.value);
		}

	//	template<>
		ExceptionMsgSink &operator <<(const VarArg<void,void> &) {
			return *this;
		}


		virtual ExceptionMsgSink &setBase(natural b) {ref.setBase(b);return *this;}
		virtual ExceptionMsgSink &setPrecision(natural p) {ref.setPrecision(p);return *this;}
		virtual ExceptionMsgSink &setSci(natural p) {ref.setSci(p);return *this;}

		ExceptionMsgSink(ExceptionMsg &ref):ref(ref) {}
		virtual ~ExceptionMsgSink() {if (!isShared()) ref.flush();}
		void flush() {ref.flush();}
	protected:
		ExceptionMsg &ref;
	};


	}

}

#endif /* LIGHTSPEED_EXCEPTIONMSG_H_ */
