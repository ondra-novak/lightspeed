/*
 * dbglog.h
 *
 *  Created on: 19.3.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_BASE_DEBUG_DBGLOG_H_
#define LIGHTSPEED_BASE_DEBUG_DBGLOG_H_

#include "../exceptions/exceptionMsg.tcc"
#include "../sync/synchronize.h"
#include "../interface.h"
#include "../containers/autoArray.h"
#include "../memory/staticAlloc.h"
#include "../text/textFormat.h"
#include "../export.h"
#include "dbglog_internals.h"

namespace LightSpeed {
	//static methods

	class StdLogOutput;

	namespace DbgLog {

		///Sets logging level
		/**
		 * Log level can be specified one of following two forms
		 *
		 * You can use keywords: "fatal","error","warning","note","progress","info","debug","all"
		 * which specifies lowest reported logging level.
		 *
		 * Or you can use mask: "FmEmWmNmPmImDm" where first letter identifies particular
		 * log level and letter "m" is placeholder for maximum sublevel for each level. If
		 * you use 0 instead "m", it disables log messages for specified level. Use '*'
		 * instead a number means, that all sublevels will be logged.
		 *
		 * Example "F*E*W*N1P1I1D0" - enables all levels for Fatal,Error,Warning, but
		 * enables only highest level for Note,Progress and Info, disables Debug complette
		 *
		 * @param logLevelDefinition log level definition
		 *
		 * @note this function sets level on ILogOutput object associated with current thread
		 * (however, the instance can be shared by all threads, see setLogProvider() )
		 * If other threads uses another ILogOutput implementations, function doesn't
		 * affect them. Then you have to change log level for every thread currently running
		 * in the application. Newly created threads are affected by this change
		 */
		void setLogLevel(ConstStrA logLevelDefinition);

		enum ThreadSpec {
			///Change is applied for newly created threads from this point
			/** This specification is not exactly correct. Change can also affect
			 * already running threads that did not used dbglog's functions yet.
			 */
			newThreadsOnly,
			///Change is applied only at current thread
			curThreadOnly,
			///Change is applied in both new and current thread
			newThreadsAndCurrentThread
		};

		///Changes log provider
		/**
		 * Log provider is specified as pointer to function which is responsible to
		 * create or access object which implementing ILogOutput. Change can be made
		 * for newly created threads or current threads or both. You cannot change
		 * log provider for another running thread. Make change before thread starts
		 * or use appropriate inter-thread communication to achieve this.
		 *
		 * @param logProviderSingletonFactory pointer to new log provider
		 * @param thrdSpec defines what is affected by this change
		 * @return returns previous log provider. If there were different log provider
		 * for current thread and new threads, function returns value for current thread
		 */
		LogOutputSingleton setLogProvider(LogOutputSingleton logProviderSingletonFactory,
							ThreadSpec thrdSpec = newThreadsAndCurrentThread);


		///Retrieves current log provider for new threads
		LogOutputSingleton getLogProvider();

		///Retrieves log output object for current thread
		/**@note function initializes dbglog for current thread if necesery */
		ILogOutput &getLogOutput();

		///Changes thread name
		/** Threads are named and everytime threads logs something, its name appears
		 * in log message. When there is no name associated with thread, dbglog
		 * creates random name (consists with numbers and letters). This function
		 * changes current thread name.
		 *
		 * @param name new thread name.
		 * @param appendId true to append threadId - it encoded using number and letters
		 * and it is designed to identification relevant log lines.
		 */
		void setThreadName(ConstStrA name, bool appendId);

		///Retrieves reference to StdLogOutput for additional configuration
		/**
		 * You can use this function to configure to which file will be target of log
		 * messages and whether stdError will be used. There is also function
		 * for rotating log.
		 *
		 * @note Function retrieves pointer to current instance implementing StdLogOutput
		 * not the singleton instance of the original object. Any object that
		 * inherits StdLogOutput can be accessed via this function to configure
		 * common features.
		 *
		 * @return pointer to StdLogOutput instance. Function can return NULL, if
		 *   active log provider is not implements StdLogOutput.
		 *
		 * @note Function uses getLogOutput() to retrieve current ILogOutput. Standard
		 * behavior is, that instance of ILogOutput is shared across all threads using
		 * the same log-provider.
		 *
		 * But this also depends on implementation of the
		 * LogOutputSingleton. Function can create instance for every thread and in this
		 * case this function configures only current thread, not other threads (However,
		 * standard implementation doesn't doing this, so there should be one instance
		 * for all threads)
		 */
		StdLogOutput *getStdLog();

		///Returns log provider which doesn't provide any logging
		/** This is effective way how to disable logging complete */
		LogOutputSingleton getNoLogProvider();

		///Rotates current log (log for current thread)
		/** @copydoc ILogOutput::logRotate */
		void logRotate();
		///Rotates current log (log for current thread)
		/** @copydoc ILogOutput::logRotate */
		void logRotate();


		///temporary redirect logs to different provider.
		/**When this object is destroyed, logs are redirected back to original provide */
		class Redirect {
		public:
			///Redirect to specified output implementation
			Redirect(ILogOutput &output);
			///Redirect to specified provider
			Redirect(LogOutputSingleton provider);
			///Stop redirect
			~Redirect();
		private:
			ILogOutput *prev;

		};
	}




	///Handles logging
	/** This class is responsible to create application logs. To use this
	 * class, you have to create instance in every function, which wants
	 * to log. You should not give this instance as argument to the other function,
	 * because every function (every level) should have own instance.
	 *
	 * Logging is very simple. you only invoke function with requested level which
	 * specifies format of log message and follows with the arguments
	 *
	 * @code
	 * LogObject log(THISLOCATION);
	 *
	 * log.fatal("Fatal program exit (example), exitcode: %1") << exitCode;
	 * log.error("This is example of error message");
	 * log.warning("Warning, program is tired, it is running for %1 hours") << hoursRunning;
	 * log.info("Nothing to do for %1 minutes") << idleTime;
	 * log(ILogOutput::logWarning,"Another way how to set log level for this message);
	 *
	 * @endcode
	 *
	 *
	 *
	 */
	class LogObject {
	public:



		typedef _intr::LogFormatter::Feed Feed;

		///Constructs log object
		/**
		 * @param loc location of place where object is constructed. Use THISLOCATION
		 */
		LogObject(const ProgramLocation &loc)
			:loc(loc),fmt(_intr::LogFormatter::getInstance())
			{intLevel(1);}
		///Constructs log object
		/**
		 * @param loc location of place where object is constructed. Use THISLOCATION
		 * @param lev how many levels skip for this instance. Default value is 1
		 */
		LogObject(const ProgramLocation &loc,natural lev)
			:loc(loc),fmt(_intr::LogFormatter::getInstance())
			{intLevel(lev);}
		///Constructs log object
		/**
		 * @param loc location of place where object is constructed. Use THISLOCATION
		 * @param reference to formatter. You can specify custom formater to redirect output or
		 *  you can write to log
		 * @param lev how many levels skip for this instance. Default value is 1
		 */
		LogObject(const ProgramLocation &loc,_intr::LogFormatter &fmt, natural lev = 1)
			:loc(loc),fmt(fmt)
			{intLevel(lev);}

		///dtor
		~LogObject() {fmt.decLevel(levelAdv);}

		///Retrieves current output object
		ILogOutput &getOutput() const {return fmt.getOutput();}

		//@{
		///Starts log message, while type is specified using variable
		/**
		 * @param type log level type
		 * @param pattern pattern of the log message.
		 * @return object that receives arguments for that pattern
		 */

		Feed  operator()(ILogOutput::LogType type, ConstStringT<char> pattern) {
			return newLogLine(type,pattern);
		}
		//@}

		///fatal message
		/** Fatal messages are reserved for serious errors which can cause
		 * that program will unable to continue or it will continue in wrong way.
		 * @param pattern message pattern, use %N as placeholder for arguments
		 * @return feed object to collect arguments
		 */
		Feed fatal(ConstStringT<char> pattern) {return newLogLine(ILogOutput::logFatal,pattern);}
		///error message
		/** All other error messages. Program meet an error condition which causes, that
		 * results are unusable. However, program can recover from the error and will continue
		 * to run as far as possible
		 * @param pattern message pattern, use %N as placeholder for arguments
		 * @return feed object to collect arguments
		 */
		Feed error(ConstStringT<char> pattern) {return newLogLine(ILogOutput::logError,pattern);}
		///warning message
		/** Reports unusual state of program, which may or may not lead to unusable results,
		 * depend on other unknown conditions. However, state of the program is stable and
		 * program can continue to run
		 * @param pattern message pattern, use %N as placeholder for arguments
		 * @return feed object to collect arguments
		 */
		Feed warning(ConstStringT<char> pattern) {return newLogLine(ILogOutput::logWarning,pattern);}
		///note message
		/** Message contain serious information about program state, which should
		 * be stored for later usage. You can use this to report result of calculation.
		 * Don't use to report progress, there is another method to do this: progress()
		 * @param pattern
		 * @return
		 */
		Feed note(ConstStringT<char> pattern) {return newLogLine(ILogOutput::logNote,pattern);}
		///progress message
		/** Message contains information about progression in the calculation.
		 * Example: "Loading files.", "Analyzing data", "Storing results". All these
		 * messages are progress type.
		 * @param pattern message pattern, use %N as placeholder for arguments
		 * @return feed object to collect arguments
		 */
		Feed progress(ConstStringT<char> pattern) {return newLogLine(ILogOutput::logProgress,pattern);}
		///info message
		/**
		 * Use to report all other information about progression. For example results of
		 * particular calculations. "Found magic AB78 at location 1036","Max value is 4564"
		 *
		 * @param pattern
		 * @return
		 */
		Feed info(ConstStringT<char> pattern) {return newLogLine(ILogOutput::logInfo,pattern);}
		///debug message
		/**
		 * Report all other messages which are not necessary to be stored in production
		 * state, but can be sometimes useful (for example when "livelog" is enabled).
		 * Examples: Queries carried on DB engine, particular results inside of cycles,
		 * progression inside of cycles.
		 *
		 *
		 * @param pattern message pattern, use %N as placeholder for arguments
		 * @return feed object to collect arguments
		 */
		Feed debug(ConstStringT<char> pattern) {return newLogLine(ILogOutput::logDebug,pattern);}


		///Test whether message on 'debug' level will be really stored
		/** you can skip some part of code if logging is disabled to enhance performance */
		bool isDebugEnabled() const {return fmt.checkLogLevel(ILogOutput::logDebug);}
		///Test whether message on 'progress' level will be really stored
		/** you can skip some part of code if logging is disabled to enhance performance */
		bool isProgressEnabled() const {return fmt.checkLogLevel(ILogOutput::logProgress);}
		///Test whether message on 'note' level will be really stored
		/** you can skip some part of code if logging is disabled to enhance performance */
		bool isNoteEnabled() const {return fmt.checkLogLevel(ILogOutput::logNote);}
		///Test whether message on 'info' level will be really stored
		/** you can skip some part of code if logging is disabled to enhance performance */
		bool isInfoEnabled() const {return fmt.checkLogLevel(ILogOutput::logInfo);}
		///Test whether message on 'warning' level will be really stored
		/** you can skip some part of code if logging is disabled to enhance performance */
		bool isWarningEnabled() const {return fmt.checkLogLevel(ILogOutput::logWarning);}
		///Test whether message on 'error' level will be really stored
		/** you can skip some part of code if logging is disabled to enhance performance */
		bool isErrorEnabled() const {return fmt.checkLogLevel(ILogOutput::logError);}
		///Test whether message on 'fatal' level will be really stored
		/** you can skip some part of code if logging is disabled to enhance performance */
		bool isFatalEnabled() const {return fmt.checkLogLevel(ILogOutput::logFatal);}


	protected:
		ProgramLocation loc;
		_intr::LogFormatter &fmt;
		natural levelAdv;

		void intLevel(natural l) {
			fmt.incLevel(l);
			levelAdv = l;
		}

		Feed newLogLine(ILogOutput::LogType type, ConstStrA pattern) {
				return fmt.initPattern(loc,type,pattern);
		}

	};


	extern LIGHTSPEED_EXPORT const char *__logMsgFormat;
	
}

#define LS_LOG ::LightSpeed::LogObject(THISLOCATION)
#define LS_LOGOBJ(x) ::LightSpeed::LogObject x(THISLOCATION)



#endif /* DBGLOG_H_ */
