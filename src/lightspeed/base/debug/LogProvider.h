/*
 * LogProvider.h
 *
 *  Created on: 11.10.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_LOGPROVIDER_H_
#define LIGHTSPEED_BASE_LOGPROVIDER_H_
#include "dbglog.h"
#include "../text/textFormat.h"
#include "../sync/threadObject.h"

namespace LightSpeed {


class AbstractLogProvider: public ILogOutput {
public:

	struct LogTimestamp {
		natural year, month, day, hour, min, sec;

		LogTimestamp();
	};

	virtual void logOutput(ConstStrA ident,
						   LogType type,
						   natural level,
						   ConstStrA text,
						   const ProgramLocation &loc);

	virtual void logOutput(ConstStrA ident,
						   LogType type,
						   natural level,
						   ConstStrA text,
						   const ProgramLocation &loc,
						   const char *format);


	virtual bool checkLogLevel(LogType type, natural level) ;
	virtual void setLogLevel(LogType type, natural maxLevel) ;

	///Sets format of log message
	/**
	 * @param format pointer to format specification. Note that pointer must point to
	 * statically allocated string, otherwise caller must ensure, that text is
	 * valid until it is changed. Function doesn't make copy of text, it is only
	 * holds reference
	 *
	 * Format uses following place-holders
	 * %1 - year
	 * %2 - month
	 * %3 - day
	 * %4 - hour
	 * %5 - minute
	 * %6 - seconds
	 * %7 - symbol (level)
	 * %8 - level
	 * %9 - thread name
	 * %10 - message
	 * %11 - function name
	 * %12 - filename
	 * %13 - line number
	 *
	 */
	void setFormat(const char *format);

	AbstractLogProvider();

	///send line to output - line doesn't contain terminating new line character
	/**
	 * sends log line to the output. It can be file, stderr, or something else.
	 * This function is considered as MT safe. If your output service is not
	 * MT safe, use Synchronized inside of implementation
	 *
	 * @param line text - whole line
	 * @param logType - type of line (major log level)
	 * @param level - level of line (minor log level)
	 */
	virtual void logLine(ConstStrA line, ILogOutput::LogType logType, natural level) = 0;


protected:


	typedef TextFormatBuff<char> Buffer;
	typedef ThreadObj<Buffer> TLSBuffer;

	TLSBuffer buffer;
	natural maxLevels[8];
	const char *format;

};

} /* namespace LightSpeed */
#endif /* LIGHTSPEED_BASE_LOGPROVIDER_H_ */
