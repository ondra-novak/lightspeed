/*
 * process.h
 *
 *  Created on: 22.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_PROCESS_H_
#define LIGHTSPEED_MT_PROCESS_H_

#include "mtRefCntPtr.h"
#include "../base/streams/fileio.h"
#include "timeout.h"
#include "../base/containers/map.h"
#include "gate.h"
#include "../base/memory/stdFactory.h"

namespace LightSpeed {

	class Gate;
	class ProcessGroup;

	///Contains internal informations about running process
	class IProcessContext: public RefCntObj {
	public:
		virtual ~IProcessContext() {}
	};

	struct ProcessEnvKeyCompare{
	public:
		bool operator()(const ConstStrW &a, const ConstStrW &b) const;
	};

	///Environment map
	/** Defines environment for the process. Every field
	 * contains key and value of the environment
	 */
	typedef Map<ConstStrW, ConstStrW, ProcessEnvKeyCompare> ProcessEnv;

	///Smart pointer to process context
	typedef MTRefCntPtr<IProcessContext> PProcessContext;


	///Creates and control process
	/** This class is used to easy create and control process. It is
	 * primary designed to execute foreign processes, but also allows
	 * to connect standard input/output to the files, pipes, connect
	 * more processes to the each other, and so on.
	 *
	 */
	class Process {

	public:

		///Creates empty process object
		/** which cannot be started, but you can use assignment operator
		 to define process name later */

		Process();
		///Creates process object
		/**
		 * @param processName pathname to process
		 */
		 
		Process(String processName);

		///Creates process object
		/**
		 * @param processName pathname to process
		 * @param alloc reference to allocator
		 */
		 
		Process(String processName, IRuntimeAlloc &alloc);

		///Destroys process object
		~Process();

		
		///Clears arguments
		Process &clear();

		///True, if process has no arguments
		bool empty() const;

		///Retrieves current process name
		const String getProcessName() const {return processName;}

		///Adds process argument;
		Process &arg(const String &arg);

		///Adds process argument;
		Process &arg(ConstStrW arg);

		///Adds process argument;
		Process &arg(const StringA &arg);
		
		///Adds process argument;
		Process &arg(ConstStrA arg);

		///Adds process argument;
		Process &arg(const char *arg);

		///Adds process argument;
		Process &arg(const wchar_t *arg);

		///Adds process argument;
		Process &arg(natural number, natural base=10);

		///Adds process argument;
		Process &arg(integer number, natural base=10);

		///Adds process argument;
		Process &arg(float number);

		///Adds process argument;
		Process &arg(double number);

		///New process breaks current job
		/** If current process is part of job, newly created process
		    will be created outside of the job.

			Under Windows, this includes CREATE_BREAKAWAY_FROM_JOB into the CreateProcess API call. 
			If current job doesn't grant breakaway permission, executing process fails.

			Under Linux, this causes configuring new job in forked child. There should be always success.
			Note that Linux always creates new job. Under Windows unbound process is created

		*/
		Process &breakJob();


		///New process should act as new terminal session
		/**
		 *  Under Windows, this includes DETACHED_PROCESS flag causing, that application will not share current console (if any)
		 *  If both breakJob() and newSession() are combined, both flags are included into CreateProcess flags 
		 *
		 *  Under Linux, new session is created on child object. 
		 *  If both breakJob() and newSession() are combined, new session is created instead job, because it is not possible to have both.
		 *  
		 */
		 
		Process &newSession();



		///Adds process extra pipe
		/**
		 * @param pipeIn variable receives stream which can
		 * be used to transfer data from the new process 
		 * to the current application.
		 */
		 
		Process &arg(SeqFileInput &pipeIn);

		///Adds process extra pipe
		/**
		 * @param pipeOut variable receives stream which can
		 * be used to transfer data from the current application 
		 * to the new application.
		 */
		Process &arg(SeqFileOutput &pipeOut);

		///Appends or replaces command line with specified text
		/**
		 * Function is useful, when class must forward command line created on other place.
		 * @param cmdLine command line in the right format
		 * @param append true to append text to current command line		
		 */
		 
		Process &cmdLine(String cmdLine, bool append = false);

		///Redirects standard output to specified stream. It can be file or write end of the pipe
		Process &stdOut(const SeqFileOutput &stream);

		///Redirects standard error to specified stream. It can be file or write end of the pipe 
		Process &stdErr(const SeqFileOutput &stream);

		///Redirects standard error and output to specified stream. It can be file or write end of the pipe 
		Process &stdOutErr(const SeqFileOutput &stream);

		///Redirects standard input to specified stream. It can be file or write end of the pipe 
		Process &stdIn(const SeqFileInput &stream);

		///Creates pipe with the process
		/** stream newly created read end, connected to process output */
		Process &stdOut(SeqFileInput &stream);

		///Creates pipe with the process
		/** stream newly created read end, connected to process error */
		Process &stdErr(SeqFileInput &stream);
	
		///Creates pipe with the process
		/** stream newly created read end, connected to process output and error */
		Process &stdOutErr(SeqFileInput &stream);

		///Creates pipe with the process
		/** stream newly created write end, connected to process input */
		Process &stdIn(SeqFileOutput &stream);

		///Redirects output of this process to input of another process
		Process &stdOut(Process &stream);

		///Redirects errors from this process to input of another process
		Process &stdErr(Process &stream);

		///Redirects outputs and errors from this process to input of another process
		Process &stdOutErr(Process &stream);

		///Redirects input from this process to output of another process
		/** 
		 *@note there is no function to redirect input to error 
		   of another process. You can simply swap processes and use stdErr */
		Process &stdIn(Process &stream);

		///Connects two processes with pipe
		/**
		 * Output from left process is connected to input of the right process.
		 * Function returns right reference to allow connect additional processes
		 */
		 
		Process &operator | (Process &other) {
			return other.stdIn(*this);
		}


		///Sets processes environment
		/**
		 * @param env pointer to environment. if 0 is set, environment is removed
		 *
		 * @note environment object is shared, pointer must be valid during whole
		 * life of the object
		 */
		Process &setEnv(const ProcessEnv *env);

		
		///Retrieves current command line 
		/** Result can be used with function cmdLine() */
		String getCmdLine() const;


		///Starts the process
		void start();

		///Stops the process
		/**
		 * @param force if true, uses operation system without give to process
		 * chance to do some action. if false, tries to use methods, how
		 * to give process signal to exit as soon as posible.
		 *
		 * On linux - SIGTERM is used, when force is false, otherwise SIGKILL is used
		 *
		 * On windows - by default, function search all top-level windows of the process
		 * and sends WM_CLOSE to them. For console process, it uses GenerateCtrlEvent
		 * to generate Ctrl+Break action. If force is true, function calls TerminateProcess
		 */
		void stop(bool force = false);

		///Waits for process exit
		/** returns exitCode */
		integer join();

		///Waits for process exit until timeout elapses
		/**
		 * @param tm specifies timeout. Note, at linux platform, function
		 * don't need to return immediately when process exits, because this
		 * function is emulated using repeatedly pool of the process state
		 * interleaved with sleep of small amount of time.
		 *
		 * @retval true join success
		 * @retval false timeout
		 */
		bool join(const Timeout &tm);


		///Retrieves gate object which is opened when process is not running
		/**
		 * You can use Gate object to perform additional synchronization.
		 * Advantage of Gate object is, that you can wait on multiple gates
		 * at once. This allows to wait for multiple processes at once.
		 *
		 * @return reference to the gate object, valid for a whole lifetime
		 * of this object
		 *
		 * @note Because process cannot notify all waiting objects registered
		 * at the Gate, this function can work only if there is
		 * a thread waiting on process termination inside of function join(),
		 * because this thread can open the gate when termination is notified.
		 *
		 * You cannot start process and wait on Gate object if there is no
		 * other thread executing join(). This causes deadlock, because there
		 * is nobody which can open the Gate.
		 */
		Gate &getJoinObject() {return finishGate;}

		///Detaches the process leaving it alone
		/**
		 * Detach is useful, when you need to exit current process without
		 * waiting for child process. Child process can continue working as
		 * orphaned process. Note that if process exits before current process,
		 * it can create zombies as result (in linux).
		 */
		void detach();


		///Retrieves exit code
		/**
		 * @return exit code. Process must be joined before you can obtain
		 * exit code. In linux, process exited abnormally (through signal)
		 * returns negative exit code, where value represents signal code.
		 * For example exit code -11
		 * is abnormal termination due signal SIGSEGV
		 *
		 * Exit code of running process is undefined
		 */
		integer getExitCode() const;


		bool isRunning() const;

		///Retrieves current process's environment.
		/**
		 * @return environment for current process
		 *
		 * @note Current process environment cannot be modified, but you can create infinite
		 * instances of ProcessEnv which can be modified and can be associated with newly created
		 * process. This function creates copy of current environment.
		 *
		 * @note Don't use environment functions that are able to modify current process environment.
		 * This function can cause undefined behavior when current environment is changed by such a functions.
		 */
		 
		static ProcessEnv getEnv();

		///Sets current working directory for the process
		/**
		 * New working directory is applied to newly created process
		 */
		 
		Process &workDir(ConstStrW path) {
			cwd = path;
			return *this;
		}
		
		///Executes process and waits for its finish
		natural exec() {
			start();return join();
		}

	protected:

		String processName;
		String cwd;
		Gate finishGate;
		PProcessContext context;

	};


	class UnableToStartProcessException: public ErrNoWithDescException {
	public:

		LIGHTSPEED_EXCEPTIONFINAL;

		UnableToStartProcessException(const ProgramLocation &loc,
			int errn, String cmdLine):ErrNoWithDescException(loc, errn,cmdLine) {};

		static LIGHTSPEED_EXPORT const char *msgText;;

	protected:

		void message(ExceptionMsg &msg) const {
			msg(msgText) << this->desc;
			ErrNoException::message(msg);
		}
	};
}


#endif /* LIGHTSPEED_MT_PROCESS_H_ */
