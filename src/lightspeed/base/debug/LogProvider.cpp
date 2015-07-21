/*
 * LogProvider.cpp
 *
 *  Created on: 11.10.2013
 *      Author: ondra
 */

#include "LogProvider.h"
#include <string.h>

#include "../../mt/atomic.h"
#ifdef LIGHTSPEED_PLATFORM_WINDOWS
#include <time.h>
inline int localtime_r(const time_t *tmt, struct tm *tminfo) {
	return localtime_s(tminfo,tmt);
}
#endif


namespace LightSpeed {


void AbstractLogProvider::logOutput(ConstStrA ident, LogType type,
		natural level, ConstStrA text, const ProgramLocation& loc) {
	logOutput(ident,type,level,text,loc,this->format);
}

void AbstractLogProvider::logOutput(ConstStrA ident, LogType type,
		natural level, ConstStrA text, const ProgramLocation& loc,
		const char* format) {

	if (checkLogLevel(type,level) ) {

		checkLogRotate();

		ITLSTable &tls = ITLSTable::getInstance();
		Buffer &curbuf = buffer[tls];

		char symbs[] = {'F','E','W','N','P','I','D','?'};

		LogTimestamp tms;

		char symb = (type < logFatal || type > logDebug)?symbs[7]:symbs[type-1];

		try {
			ConstStrA fmt(format);
			curbuf(fmt)
				<< tms.year << tms.month << tms.day << tms.hour << tms.min << tms.sec
				<< symb << level << ident << text << loc.function << loc.file << loc.line;

			logLine(curbuf.write(),type,level);
		} catch (...) {


	}
	}



}

bool AbstractLogProvider::checkLogLevel(LogType type, natural level) {
	return level <= maxLevels[type % 8] ;
}

void AbstractLogProvider::setLogLevel(LogType type, natural maxLevel) {
	maxLevels[type % 8] = maxLevel;

}

void AbstractLogProvider::setFormat(const char* format) {
	this->format = format;
}

AbstractLogProvider::AbstractLogProvider() {
	memset(maxLevels,0xFF,sizeof(maxLevels));
	format  = __logMsgFormat;
	DbgLog::needRotateLogs(rotateCounter);
}



AbstractLogProvider::LogTimestamp::LogTimestamp() {
	time_t tmt;
	time(&tmt);
	struct tm tminfo;
	localtime_r(&tmt,&tminfo);
	year = tminfo.tm_year+1900;
	month = tminfo.tm_mon+1;
	day = tminfo.tm_mday;
	hour = tminfo.tm_hour;
	min = tminfo.tm_min;
	sec = tminfo.tm_sec;
}

void AbstractLogProvider::checkLogRotate() {
	if (rotateCounter != DbgLog::rotateCounter)
		if (DbgLog::needRotateLogs(rotateCounter)) {
			try {
				logRotate();
			} catch (std::exception &e) {
				logOutput("logrotate",logFatal,1,StringA(ConstStrA("Exception during logRotate(): ")+ConstStrA(e.what())),THISLOCATION);
			}
		}
}

} /* namespace LightSpeed */

