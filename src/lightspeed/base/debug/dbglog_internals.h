/*
 * dbglog_internals.h
 * Internal definitions supporting dbglog.h. Please see dbglog.h for documentation.
 * Do not use object declared here directly
 */

#ifndef LIGHTSPEED_BASE_DBGLOG_INTERNALS_H_
#define LIGHTSPEED_BASE_DBGLOG_INTERNALS_H_
#include "../interface.h"
#include "../text/textFormat.h"
#include "../memory/smallAlloc.h"

namespace LightSpeed {


class ILogOutput;

typedef ILogOutput &(*LogOutputSingleton)();

class ILogOutput: public IInterface {
public:


	enum LogType {
		///Fatal error - cannot continue
		logFatal = 1,
		///Error - recoverable
		logError = 2,
		///Warning - something can be wrong
		logWarning = 3,
		///Note - state of program can be important
		logNote = 4,
		///Progress - state of program has been changed - advanced
		logProgress = 5,
		///Info - other information
		logInfo = 6,
		///Debug info
		logDebug = 7,
	};


	/**
	 * @param ident report identifier - identifies process, thread, or pid.
	 * @param type report type
	 * @param level report level
	 * @param text description of the report
	 * @param loc location of place, where report has been generated
	 */
	virtual void logOutput(ConstStrA ident,
						   LogType type,
						   natural level,
						   ConstStrA text,
						   const ProgramLocation &loc) = 0;

	/**
	 * @param ident report identifier - identifies process, thread, or pid.
	 * @param type report type
	 * @param level report level
	 * @param text description of the report
	 * @param loc location of place, where report has been generated
	 * @param format format specification by TextFormat class.
	 *
	 * Variables:
	 * %1 - year, %2 - month, %3 - day, %4 - hour, %5 - minute, %6 - second, %7 - type
	 * %8 - level, %9 - thread ident, %10 - message, %11 - function, %12 - file, %13 - line
	 */
	virtual void logOutput(ConstStrA ident,
						   LogType type,
						   natural level,
						   ConstStrA text,
						   const ProgramLocation &loc,
						   const char *format) = 0;

	///Checks, whether message with this level will be really logged
	/**
	 * @param type log level type
	 * @param level log level value
	 * @retval true logging allowed
	 * @retval false logging disabled
	 */
	virtual bool checkLogLevel(LogType type, natural level) = 0;

	virtual void setLogLevel(LogType type, natural maxLevel) = 0;

	void setLogLevel(ConstStrA levelDesc);

	virtual ~ILogOutput() {}

	///Sets function which will be used to retrieve current logging output
	/**
	 * @param fn pointer to function to initialize and return
	 * log-output. Parameter can be NULL, then standard logging is
	 * used.
	 *
	 * Function is called every-time logging in new thread is requested
	 * Function should access singleton, which is created for first
	 * time access and then it returns same instance. Best for this
	 * is use class Singleton to create and access singleton object
	 * Result of Singleton creation should be returned as return
	 * value form such a function.
	 *
	 * Function can be called anytime before program exits or before
	 * function reference is changed. You have to be prepared, that
	 * function can be called during program exit after logging
	 * singleton has been destroyed
	 *
	 * @note function is called for every logging request. It
	 * must be prepared that it will be called from various threads
	 *
	 * To simplify creation, use pointer to Singleton::castInstance as parameter
	 *
	 * @note this function affects all threads created after this point. Function
	 * doesn't affect current tread
	 */

	static void setLogOutputSingleton(LogOutputSingleton fn);

	///Retrieves pointer to instance for current logging output singleton
	static LogOutputSingleton getLogOutputSingleton();

	///You can change log singleton inside another module
	virtual void vtSetLogOutputSingleton(LogOutputSingleton fn) {
			setLogOutputSingleton(fn);
	}

	///Retrieves pointer to instance for current logging output singleton
	virtual LogOutputSingleton vtGetLogOutputSingleton() {
	 return getLogOutputSingleton();
	}

	///call this function to reinitialize logging after logrotate request
	/**
	 * This function is no meaning, if logs are not rotated. Otherwise
	 * it notifies underlying logging system, that logs has been or will
	 * be rotated. Logging system should detach old log file and create
	 * new one.
	 *
	 * In most cases, log file is renamed and then logrotate is issued.
	 * Logging system closes old (renamed) log and creates new one.
	 *
	 * Depend on platform, this can be also implemented by different way,
	 * such a creating new log file with timestamp of creation.
	 *
	 * If logging system doesn't support rotating logs, this function
	 * should be empty
	 */
	virtual void logRotate() = 0;

};

template<typename H, typename T>
struct VarArg;

namespace _intr {

///Responsible to format message before it is sent to the ILogOutput
/**
 * There is an instance per sngle thread. You often don't use this class
 * directly. Instead, you should use LogObject to access this class
 * from various places of the program
 */
class LogFormatter {
public:

	friend class Feed;

	class Feed: public SharedResource {
	public:
		template<typename T>
		Feed &operator <<(T a) {if (!silent) ref.arg(a); return *this;}

		template<typename V, typename H>
		Feed &operator <<(const VarArg<V,H> &a) {
			operator << (a.next);
			return operator << (a.value);
		}
		Feed &operator <<(const VarArg<void,void> &) {
			return *this;
		}

		Feed(LogFormatter &ref, bool silent):ref(ref),silent(silent) {}
		virtual ~Feed() {if (!isShared() && !silent) ref.flush();}
	protected:
		LogFormatter &ref;
		bool silent;
	};


	typedef ILogOutput::LogType LogType;


	LogFormatter();
	LogFormatter(const StringA &ident, ILogOutput &output);
	LogFormatter(const StringA &ident);
	LogFormatter(const LogFormatter &other);
	virtual ~LogFormatter() {}

	ConstStrA getText() const {return ConstStrA(fmt.getBuffer());}
	template<typename T>
	void arg(const T &a) {fmt << a;}

	void incLevel(natural lev) {level+=lev;}
	void decLevel(natural lev) {level-=lev;}

	void setThreadIdent(StringA ident) {this->ident = ident;}
	StringA getThreadIdent() const {
		if (ident.empty()) buildThreadIdent();
		return ident;
	}

	virtual LogFormatter &flush() {
		output->logOutput(getThreadIdent(),logType,level,fmt.write(),loc);
		return *this;
	}

	Feed initPattern(const ProgramLocation &loc, LogType type, ConstStrA pattern) {
		this->loc = loc;
		this->logType = type;
		if (output->checkLogLevel(type,level)) {			
			fmt(pattern);
			return Feed(*this,false);
		} else {
			return Feed(*this,true);
		}


	}

	static LogFormatter &getInstance();

	ILogOutput &getOutput() const {return *output;}

	///changes current ILogOutput. Function uses singleton to retrieve pointer to new output
	void setOutput(LogOutputSingleton fn) {output = &fn();}

	void setOutput(ILogOutput &output) {this->output = &output;}

	///Checks, whether message with this level will be really logged
	/**
	 * @copydoc ILogOutput::checkLogLevel
	 */
	bool checkLogLevel(LogType type) {
		return output->checkLogLevel(type,level);
	}

protected:
	ILogOutput *output;
	LogType logType;
	natural level;
	mutable StringA ident;
	ProgramLocation loc;
	TextFormatBuff<char, SmallAlloc<1024> > fmt;

	void buildThreadIdent() const;

};

}

}



#endif /* LIGHTSPEED_BASE_DBGLOG_INTERNALS_H_ */
