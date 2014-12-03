/*
 * exceptionMsg.tcc
 *
 *  Created on: 24.1.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_EXCEPTION_EXCEPTIONMSG_TCC_
#define LIGHTSPEED_EXCEPTION_EXCEPTIONMSG_TCC_

#include "../containers/autoArray.h"
#include "../memory/smallAlloc.h"
#include "../text/textFormat.tcc"
#include "exceptionMsg.h"

namespace LightSpeed {


template<natural bufferSize>
class ExceptionMsgImplBase1: public ExceptionMsg {
	typedef SmallAlloc<bufferSize> Allocator;
	typedef AutoArrayStream<wchar_t,  Allocator> Buffer;
	typedef TextFormat<wchar_t,Allocator> Formatter;
public:

	ExceptionMsgImplBase1()
		:charBuff(Allocator())
		,formater(Allocator()) {}

	virtual _intr::ExceptionMsgSink operator()(ConstStringT<char> pattern) {
		formater(pattern);return _intr::ExceptionMsgSink(*this);
	}
	virtual _intr::ExceptionMsgSink operator()(const char *pattern){
		formater(pattern);return _intr::ExceptionMsgSink(*this);
	}
	virtual _intr::ExceptionMsgSink operator()(ConstStringT<wchar_t> pattern) {
		formater(pattern);return _intr::ExceptionMsgSink(*this);
	}
	virtual _intr::ExceptionMsgSink operator()(const wchar_t *pattern) {
		formater(pattern);return _intr::ExceptionMsgSink(*this);
	}
	virtual ExceptionMsg &setBase(natural b) {
		formater.setBase(b);return *this;
	}
	virtual ExceptionMsg &setPrecision(natural p){
		formater.setPrecision(p);return *this;
	}
	virtual ExceptionMsg &setSci(natural p){
		formater.setSci(p);return *this;
	}
	void resetArgs() {
		formater.resetArgs();
	}
	virtual ExceptionMsg &setNumberFormat(INumberFormatter *fmt) {
		formater.setNumberFormat(fmt);
		return *this;
	}
	virtual ExceptionMsg &arg(ConstStringT<char> text) {
		formater << text;return *this;
	}
	virtual ExceptionMsg &arg(const char *text) {
		formater << text;return *this;
	}
	virtual ExceptionMsg &arg(char chr) {
		formater << chr;return *this;
	}
	virtual ExceptionMsg &arg(ConstStringT<wchar_t> text) {
		formater << text;return *this;
	}
	virtual ExceptionMsg &arg(const wchar_t *text) {
		formater << text;return *this;
	}
	virtual ExceptionMsg &arg(wchar_t chr) {
		formater << chr;return *this;
	}
	virtual ExceptionMsg &arg(lnatural num) {
		formater << num;return *this;
	}
	virtual ExceptionMsg &arg(linteger num){
		formater << num;return *this;
	}
	virtual ExceptionMsg &arg(float num){
		formater << num;return *this;
	}
	virtual ExceptionMsg &arg(double num){
		formater << num;return *this;
	}
	virtual ExceptionMsg &flush() {
		formater.output(charBuff);return *this;
	}

	String getString() const {return String(charBuff.getArray());}

protected:

	Buffer charBuff;
	Formatter formater;

};

template<natural bufferSize>
class ExceptionMsgImplBase2: public ExceptionMsgImplBase1<bufferSize> {
public:
	virtual ExceptionMsg &arg(natural num) {
		this->formater << num;return *this;
	}
	virtual ExceptionMsg &arg(integer num){
		this->formater << num;return *this;
	}
};

template<natural bufferSize>
class ExceptionMsgImpl: public ExceptionMsgImplBase2<bufferSize> {
public:
	using  ExceptionMsgImplBase2<bufferSize>::arg;
	virtual ExceptionMsg &arg(int num) {
		this->formater << (integer)num;return *this;
	}
	virtual ExceptionMsg &arg(unsigned int num){
		this->formater << (natural)num;return *this;
	}
};


}

#endif /* LIGHTSPEED_EXCEPTION_EXCEPTIONMSG_TCC_ */
