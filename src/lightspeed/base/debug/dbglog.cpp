/*
 * dbglog.cpp
 *
 *  Created on: 19.3.2010
 *      Author: ondra
 */

#include <time.h>
#include "dbglog_internals.h"
#include "../memory/singleton.h"
#include "../interface.tcc"
#include "stdlogoutput.h"
#include "../../mt/thread.h"
#include "../text/textParser.tcc"
#include "../text/textFormat.tcc"


namespace LightSpeed {

static ThreadVarInitDefault<_intr::LogFormatter> globalFormatter;
static LogOutputSingleton defaultLogOutput = 0;




inline ILogOutput *getDefaultOutput() {
	if (defaultLogOutput) return &defaultLogOutput();
	else return  &Singleton<StdLogOutput>::getInstance();
}


const char *__logMsgFormat = "%{04}1/%{02}2/%{02}3 %{02}4:%{02}5:%{02}6 %7%%8: [%9] %10 (%12:%13)\n";

namespace _intr {

LogFormatter & LogFormatter::getInstance()
{
	return globalFormatter[ITLSTable::getInstance()];
}



LogFormatter::LogFormatter():output(getDefaultOutput()),level(0) {

}

LogFormatter::LogFormatter(const StringA &ident, ILogOutput &output)
	:output(&output),level(0),ident(ident) {


}

LogFormatter::LogFormatter( const StringA &ident )
	:output(getDefaultOutput()),level(0),ident(ident) {

}

LogFormatter::LogFormatter( const LogFormatter &other )
	:output(getDefaultOutput()),level(other.level)
{
}

void LogFormatter::buildThreadIdent()
{
	TextFormatBuff<char, StaticAlloc<256> > fmt;
	if (!Thread::isThreaded() || Thread::current().getThreadId() == Thread::getMaster().getThreadId()) {
		ident="main";
	} else {
		fmt.setBase(36);
		fmt("%1") << ThreadId::current().asAtomic();
		ident = fmt.write();
	}
}
}




void ILogOutput::setLogLevel(ConstStrA levelDesc) {
	if (levelDesc == ConstStrA("fatal")) {
		setLogLevel("F*E0W0N0P0I0D0");
	} else if (levelDesc == ConstStrA("error")) {
		setLogLevel("F*E*W0N0P0I0D0");
	} else if (levelDesc == ConstStrA("warning")) {
		setLogLevel("F*E*W*N0P0I0D0");
	} else if (levelDesc == ConstStrA("note")) {
		setLogLevel("F*E*W*N*P0I0D0");
	} else if (levelDesc == ConstStrA("progress")) {
		setLogLevel("F*E*W*N*P*I0D0");
	} else if (levelDesc == ConstStrA("info")) {
		setLogLevel("F*E*W*N*P*I*D0");
	} else if (levelDesc == ConstStrA("debug")) {
		setLogLevel("F*E*W*N*P*I*D*");
	} else if (levelDesc == ConstStrA("all")) {
		setLogLevel("F*E*W*N*P*I*D*");
	} else {
		for(ConstStrA::Iterator iter = levelDesc.getFwIter(); iter.hasItems();) {
			char code = iter.getNext();
			natural level;
			bool numb = parseUnsignedNumber(iter,level,10);
			if (numb == false && iter.hasItems()) {
				char a = iter.getNext();
				if (a == '*') level = naturalNull;
			}
			switch (code) {
			case 'F':  setLogLevel(logFatal,level);break;
			case 'E':  setLogLevel(logError,level);break;
			case 'W':  setLogLevel(logWarning,level);break;
			case 'N':  setLogLevel(logNote,level);break;
			case 'P':  setLogLevel(logProgress,level);break;
			case 'I':  setLogLevel(logInfo,level);break;
			case 'D':  setLogLevel(logDebug,level);break;
			}
		}
	}

}


void ILogOutput::setLogOutputSingleton( LogOutputSingleton fn )
{
	if (fn == 0) setLogOutputSingleton(&Singleton<StdLogOutput>::castInstance<ILogOutput>);
	else defaultLogOutput = fn;
	
}

LightSpeed::LogOutputSingleton ILogOutput::getLogOutputSingleton()
{
	if (defaultLogOutput == 0) return &Singleton<StdLogOutput>::castInstance<ILogOutput>;
	return defaultLogOutput;
}


#if 0
LogObject::Feed & LogObject::Feed::operator<<( const DumpData &a )
{
	AutoArrayStream<char> buff;
	const char *p = reinterpret_cast<const char *>(a.startAddr);
	for (natural i = 0; i < a.length; i++) {
		if (p[i] <32) buff.write('.');else buff.write(p[i]);
	}
	buff.write(' ');
	buff.write('-');
	buff.write(' ');
	for (natural i = 0; i < a.length; i++) {
		if (i) buff.write('-');
		byte k = (byte)p[i];
		byte h = k >> 4;
		byte l = k & 0xF;
		if (h > 9) buff.write(h + 'A' - 10);
		else buff.write(h + '0');
		if (l > 9) buff.write(l + 'A' - 10);
		else buff.write(l + '0');
	}

	StringA out = buff.getArray();
	return this->operator <<(out);
}
#endif

void DbgLog::setLogLevel(ConstStrA logLevelDefinition) {
	getLogOutput().setLogLevel(logLevelDefinition);
}

LogOutputSingleton DbgLog::setLogProvider(
		LogOutputSingleton logProviderSingletonFactory, ThreadSpec thrdSpec) {

	LogOutputSingleton retVal = defaultLogOutput;

	switch (thrdSpec) {
	case newThreadsOnly:
		defaultLogOutput = logProviderSingletonFactory;break;
	case newThreadsAndCurrentThread:
		defaultLogOutput = logProviderSingletonFactory;
		/* don't break */
	case curThreadOnly: {
		_intr::LogFormatter &fmt = _intr::LogFormatter::getInstance();
		fmt.setOutput(logProviderSingletonFactory);
		}
		break;
	}

	return retVal;

}

LogOutputSingleton DbgLog::getLogProvider() {
	return defaultLogOutput;
}

ILogOutput& DbgLog::getLogOutput() {
	_intr::LogFormatter &fmt = _intr::LogFormatter::getInstance();
	return fmt.getOutput();

}

void DbgLog::setThreadName( ConstStrA ident, bool threadId  )
{
	_intr::LogFormatter &fmt = _intr::LogFormatter::getInstance();
	if (threadId && !ident.empty()) {
		TextFormatBuff<char> tfmt;
		tfmt.setBase(36);
		tfmt("%1 (%2)") << ident << ThreadId::current().asAtomic();
		fmt.setThreadIdent(tfmt.write());
	} else {
		fmt.setThreadIdent(ident);
	}
}

void DbgLog::setThreadName(const StringA &name, bool appendId) {
	if (appendId) setThreadName(ConstStrA(name), true);
	else {
		_intr::LogFormatter &fmt = _intr::LogFormatter::getInstance();
		fmt.setThreadIdent(name.getMT());
	}
}

void DbgLog::setThreadName(const char *name, bool appendId) {
	setThreadName(ConstStrA(name),appendId);
}

ConstStrA DbgLog::getThreadName(  )
{
	_intr::LogFormatter &fmt = _intr::LogFormatter::getInstance();
	return fmt.getThreadIdent();
}

StdLogOutput* DbgLog::getStdLog() {
	return getLogOutput().getIfcPtr<StdLogOutput>();
}


void DbgLog::logRotate() {
	getLogOutput().logRotate();
}

static atomic stRtCounter = 0;
const atomic &DbgLog::rotateCounter = stRtCounter;

void DbgLog::logRotateAll() {
	lockInc(stRtCounter);
}

bool DbgLog::needRotateLogs(atomic &internalcounter) {
	atomicValue curValue = internalcounter;
	atomicValue curGlobValue = stRtCounter;
	if (curGlobValue != curValue) {
		if (lockCompareExchange(internalcounter,curValue,curGlobValue) == curValue) {
			return true;
		}
	}
	return false;
}


class NoLogProvider: public ILogOutput {
public:
	virtual void logOutput(ConstStrA ,
						   LogType ,
						   natural ,
						   ConstStrA ,
						   const ProgramLocation &) {}

	virtual void logOutput(ConstStrA ,
						   LogType ,
						   natural ,
						   ConstStrA ,
						   const ProgramLocation &,
						   const char *) {}

	virtual bool checkLogLevel(LogType , natural ) {return false;}

	virtual void setLogLevel(LogType , natural ){}
	virtual void logRotate(){}



};

LogOutputSingleton LightSpeed::DbgLog::getNoLogProvider() {
	return &Singleton<NoLogProvider>::castInstance<ILogOutput>;
}

LightSpeed::DbgLog::Redirect::Redirect(ILogOutput& output) {
	_intr::LogFormatter &fmt = _intr::LogFormatter::getInstance();
	prev = &fmt.getOutput();
	fmt.setOutput(output);

}

LightSpeed::DbgLog::Redirect::~Redirect() {
	_intr::LogFormatter &fmt = _intr::LogFormatter::getInstance();
	fmt.setOutput(*prev);
}

LightSpeed::DbgLog::Redirect::Redirect(LogOutputSingleton provider) {
	_intr::LogFormatter &fmt = _intr::LogFormatter::getInstance();
	prev = &fmt.getOutput();
	fmt.setOutput(provider);
}

}

