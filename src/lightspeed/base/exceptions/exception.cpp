/*
 * exception.cpp
 *
 *  Created on: 4.9.2009
 *      Author: ondra
 */

#include "exception.h"
#include "../containers/string.tcc"
#include "../streams/utf.h"
#include <string.h>
#include "stdexception.h"
#include "exceptionMsg.tcc"
#include "../../base/framework/iapp.h"
#include "../../base/sync/threadVar.h"



namespace LightSpeed {


static const natural exceptionSmallBuffer = 2048;

void firstChanceException(const Exception *exception) ;
void exceptionDestroyed(const Exception *exception) ;



Exception::Exception(const ProgramLocation & loc)
    :location(loc)
{
	firstChanceException(this);
//	debugBreak();
}

Exception::Exception(const Exception & other)
:location(other.location),reason(other.reason.getMT())
{

}

Exception::~Exception() throw() {
	exceptionDestroyed(this);
}


const char *Exception::what() const throw ()
{
	if (whatMsg.getSize() == 0) {
		String msg = getLongMessage();
		StringA amsg = msg.getUtf8();

		StdAlloc::AllocatedMemory<char> tmp(amsg.length()+1);
		for (natural i = 0; i < amsg.length();i++) {
			tmp.getBase()[i] = amsg[i];
		}
		tmp.getBase()[amsg.length()] = 0;
		tmp.swap(whatMsg);
	}
	return whatMsg.getBase();
}


String Exception::getMessage() const
{
	ExceptionMsgImpl<exceptionSmallBuffer> msg;
    message(msg);
    return msg.getString();

}

const Exception *Exception::getReason() const
{
    return reason;
}



const char *Exception::getStaticDesc() const
{
    return "This execption has no static description";
}

/*Exception::LocationIterator Exception::getTrackedLocations() const
{
    return locationTracker.getIterator();
}*/



void Exception::setReason(const PException & rs)
{
    appendReason(rs);
}



void Exception::setReason(const Exception & rs)
{
    setReason(rs.safeClone());
}

void Exception::appendReason(const PException & rs)
{
	if (this->reason == nil)
		this->reason = rs;
	else {
		Exception *iter = this->reason;
		while (iter->reason != 0) iter = iter->reason;
		iter->reason = rs;
	}
}



void Exception::appendReason(const Exception & rs)
{
	appendReason(rs.safeClone());
}
void Exception::throwAgain(const ProgramLocation & )
{
//TODO:    locationTracker.write(loc);
    throwAgain();
}

const ProgramLocation & Exception::getLocation() const
{
    return location; 
}

const wchar_t *Exception::str_because = L", because:";
const wchar_t *Exception::str_exception = L": Exception: ";
const wchar_t *Exception::str_rethrownhere = L": re-thrown here.";



String Exception::getLongMessage() const
{
    ExceptionMsgImpl<exceptionSmallBuffer> msg;
    msg("%1(%2)%3") << location.file << location.line << str_exception;

    message(msg);

    msg(" (%1, %2)") << typeid(*this).name() << location.function;

	String longMsg;
    if (reason != nil) {
		longMsg = reason->getLongMessage();
    	msg("%1\n%2") << str_because << longMsg;
    }

    /* TODO
	LocationIterator iter = getTrackedLocations();
	while (iter.hasItems()) {
		const ProgramLocation &loc = iter.getNext();
		msg("\n%1(%2)%3")<<loc.file << loc.line << str_rethrownhere;
	}
	*/

    return msg.getString();
}

String Exception::getMessageWithReason() const
{
	ExceptionMsgImpl<exceptionSmallBuffer> msg;
	message(msg);
	String t;
	if (reason != nil) {
		t = reason->getMessageWithReason();
		msg("%1\n%2") << str_because << t;
	}

	return msg.getString();

}

PException Exception::safeClone()  const throw()
{
    return (Exception *)clone();
}


void Exception::trackThrow(const ProgramLocation &) {
//TODO:    locationTracker.write(loc);
    throw;
}

void Exception::track(const ProgramLocation &) {
//TODO:	   locationTracker.write(loc);
}


bool Exception::setLocation(const ProgramLocation &loc) {
	if (location.isUndefined()) {
		location = loc;
		IApp *app = IApp::currentPtr();
		if (app) app->onFirstChanceException(this);
		return true;
	} else
		return false;
}

void Exception::rethrow(const ProgramLocation &loc) {

	try {
		throw;
	} catch (Exception &e) {
		e.trackThrow(loc);
	} 
	throw;
}

PException Exception::getCurrentException() {
	try {
		throw;
	} catch (Exception &e) {
		return PException(e.clone());
	} catch (std::exception &e) {
		return new StdException(THISLOCATION,e);
	} catch (...) {
		return new UnknownException(THISLOCATION);
	}

}


// namespace LightSpeed
}
