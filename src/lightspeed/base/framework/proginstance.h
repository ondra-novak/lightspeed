/** @file
 * Copyright (c) 2006, Seznam.cz, a.s.
 * All Rights Reserved.
 * 
 * $Id: proginstance.h 721 2015-07-13 10:57:39Z bredysoft $
 *
 * DESCRIPTION
 * Short description
 * 
 * AUTHOR
 * Ondrej Novak <ondrej.novak@firma.seznam.cz>
 *
 */

#ifndef SZN_FETCHSTATS_PROGINSTANCE_H_
#define SZN_FETCHSTATS_PROGINSTANCE_H_

#include "../containers/string.h"
#include "../exceptions/exception.h"
#include "../exceptions/systemException.h"
#include "../export.h"
namespace LightSpeed {


struct ProgInstanceBuffer;
///Defines program instance for Interprocess communication with other processes
/**
 * This class handles two problems. First implements PID file, which
 * is used to identify program instance and allows to prevent multiple running
 * of single instance of the program. Second, it implements simple mailbox for
 * controlling the process without need to use signals.
 *
 * The implemented mailbox is very simple and allows to hold only one request
 * at time. It always blocking - requesting process must wait for
 * reply. During waiting, other processes must wait until current request
 * is processed.
 *
 * Called process can periodically check the mailbox, or can wait for message
 * while it is blocked. Once request arrives, it can accept the request,
 * read the request and then, write the reply and send the reply to the
 * waiting process.
 *
 */
class  ProgInstance {
public:
	ProgInstance(const String &name);
	~ProgInstance();

	ProgInstance(const ProgInstance &other);


	///Creates program instance file
	/**
	 * Handles creation of instance file. If file exists, it
	 * checks, whether belongs to running process
	 *
	 * @exception AlreadyRunningException_t process already running
	 * @exception SystemException thrown, when file cannot be created
	 */
	void create();

	///Creates program instance file
	/**
	 * Handles creation of instance file. If file exists, it
	 * checks, whether belongs to running process
	 *
	 * @param requestMaxSize defines size of request in bytes. Calling
	 * this function without parameter, object will initialize this
	 * property to use remaining space of instance-file, which can be
	 * near to 4KB
	 *
	 * @exception AlreadyRunningException_t process already running
	 * @exception SystemException thrown, when file cannot be created
	 */
	void create(natural requestMaxSize);


	///Opens program instance file for requests
	/**
	 * @exception NotRunningException_t process is not running
	 * @exception SystemException thrown, when file cannot be opened
	 */
	void open();


	///checks instance file without opening the instance
	/** 
	 @retval true checked OK
	 @retval false instance file is invalid or cannot be  opened
	 */
	bool check() const;

	///Requests running processes
	/**
	 * @param req pointer to memory containing the request data. Function
	 * 	will copy request to the target process and sends notification to
	 * 	the process about the pending request.
	 * @param reqsize size of request in the bytes. This value must
	 * 	be less that reserved space in the target buffer. You can
	 * 	receive this value by calling function getRequestMaxSize()
	 * @param reply pointer to buffer, which receives reply. After
	 * 	request is processed, reply is copied from target process memory
	 * 	to the specified buffer
	 * @param replySize size of buffer in the bytes
	 * @param timeout timeout in milliseconds
	 * @retval count of bytes copied to the reply.
	 * @exception TimeoutException_t request timeouted
	 * @exception RequestToLarge_t request is too large
	 * @exception ReplyBufferIsTooSmall_t there is no space to store reply
	 */
	natural request(const void *req, natural reqsize,
				 void *reply, natural replySize,
				 natural timeout);


	///Retrieves maximal size of the request
	natural getRequestMaxSize();
	///Retrieves maximal size of the reply
	/**
	 * Because buffer for reply is same to buffer to request, this function
	 * will return same value as getRequestMaxSize()
	 * @return maximal size of reply
	 */
	natural getReplyMaxSize();

	///Tests, whether there is request ready
	bool anyRequest();


	///Accepts request
	/**
	 * @retval 0 no request is ready
	 * @retval other pointer to request
	 *
	 * When program accept the request, it must reply the request as soon
	 * as possible. To retrieve size of request, use getRequestSize().
	 *
	 * When request is accepted, caller can modify the data at returned
	 * address to make reply from the request.
	 *
	 */
	void *acceptRequest();


	///Waits for request
	/**
	 * Pauses process until the request arrives
	 * @param timeout timeout in milliseconds. Use naturalNull to infinite waiting
	 * @retval ptr request arrived
	 * @retval 0 timeout or request has been canceled 
	 * @exception ErrNoException error while waiting.
	 *
	 * @note accepts request automatically, you should not call acceptRequest
	 *
	 * @par Platform specifics
	 * Messages posted to the windows attached to the current thread
	 * are dispatched under Microsoft Windows. Do not create windows in
	 * this thread if you need to disable this feature. 
	 *
	 *  @see cancelWaitForRequest
	 */
	void *waitForRequest(natural timeout);


	///Retrieves buffer where to store reply
	/**
	 * This function is valid only when master process processing the
	 * request. Process can store reply to this buffer and call sendReply()
	 * function, where size of reply is specified. Maximal size of reply
	 * can be retrieved by getReplyMaxSize()
	 */
	void *getReplyBuffer();

	///Sends reply
	/**
	 * @param sz size of reply. Reply must be stored in the buffer
	 * which address has been returned by acceptRequest(). Data in the
	 * buffer must be changed to reply.
	 */
	void sendReply(natural sz);

	///Sends reply
	/**
	 * @param data pointer to data containg reply. Function will copy the
	 * data into request/reply buffer
	 * @param sz size of buffer
	 */
	void sendReply(const void *data, natural sz);

	///Tests, whether objects owns the instance file
	/**
	 * Owner is the application, which created the instance file
	 *
	 * @retval true yep, I am owner
	 * @retval false no, I am not owner
	 */
	bool imOwner() const;


	///Takes ownership of the instance file
	/** Becomes owner. You can take ownership only if the
	 * the application is child, or daemon, so there has been create()
	 * invoked as different process. You cannot take ownership after open().
	 *
	 * This function should be called after daemon() command issued. Under
	 * windows environment, you cannot start application as a daemon(),
	 * but function also returns success, if called in ownership process
	 *
	 * @return true success, current process owns the instance file
	 * @retval false failed, cannot take ownership
	 * @exception SystemException any error
	 */
	bool takeOwnership();



	///Waits for process termination
	/**
	 * Needs opened instance. Function closes instance and waits for
	 * process termination. You can specify timeout to wait.
	 * @param timeout count of miliseconds to wait. Depend on implementation,
	 * this value can be rounded to nearest half-second (linux)
	 *
	 * @note Function doesn't send signal to the application to termination. You
	 * have to use proper IPC to request application to terminate. This
	 * function only ensures, that process has already exited and you
	 * can create new instance with the same name.
	 *
	 * On some platforms, dots can be sent to the output
	 */
	void waitForTerminate(natural timeout);

	///Enters daemon mode
	/** Uses best way implement daemon mode. Under Linux, fork is used
	and then application detaches itself from the console. 
	Under Windows, function restarts application with same arguments
	but uses environment variable to notify, that second instance
	is started. Function expects, that second instance also reaches
	this function and will be able to determine, that application
	is already in daemon mode. First instance exits prematurely without
	returning from this function 

	You should not perform any non repeatable tasks before application
	enter to daemon mode, because on some platforms means, that
	these tasks will be repeated

        @param restartOnErrorSec allows to restart deamon mode when 
	application crashes. Default value 0 disables this feature.
	Other values specifies number of seconds wait before next restart
	Service is restarted only when critical error, unhandled exception or signal
	is caught. On some platforms, interval can be ignored and
	feature is enabled for any value above zero. If platform
	doesn't support restarts, or restarts are handled by
	outside of application, this argument is ignored
	*/
	void enterDaemonMode(natural restartOnErrorSec = 0);
	
	///Retrives count of restarts in the demond mode
	/**
	 * @return returns 0 when application is running smoothly or any number
	 * above 0 if there is detected restarts.
	 *
	 * @note not all platforms can support this. Function returns naturalNull
	 * if number is not know
	 */
	static natural getRestartCounts() ;


	///Retrieves count of seconds, which instance is running
	/**
	 *
	 * @param resetOnRestart true if you want to reset number to zero on
	 *   restart.
	 * @return count of seconds from start (or restart)
	 *
	 * @note function returns valid number after enterDaemonMonde
	 */
	static natural getUpTime(bool resetOnRestart) ;

	///Initializes variable that hold's program start time.
	/** You need to call this function when you plan to use function
	 * getUpTime without initializing the ProgInstance object and entering
	 * to daemon mode. Function performs initialization only for first calling,
	 * any other calls does nothing.
	 */
	static void initializeStartTime();

	///gets total CPU time consumed by this process
	/**
	 * @return total CPU time in milliseconds. For multicore CPUs, returns sums through all cores
	 */
	static natural getCPUTime();

	///gets process memory usage
	/**
	 * @return returns net memory usage. It excludes memory allocated by the allocator
	 * itself and memory unavailable due memory fragmentation.
	 *
	 * @note due lack of API under Linux environment, returned value may be inaccurate
	 * when process takes more then 2GB of the memory.
	 */
	static natural getMemoryUsage();


	void restartDaemon();

	///Determines, whether instance is started in daemon mode
	/**
	 * @retval true instance is in daemon mode.
	 * @retval false instance is not in daemon mode
	 *
	 * @note Function should determine daemon mode from current object. If 
	 * application is in daemon mode but not through this object, function should
	 * return false
	 *
	 * @note Under Windows and generally under platform without fork() implementation,
	 * the daemon mode is reported after program enters to the daemon mode and
	 * spawns the daemon process. The newly spawned process is daemon and function
	 * will return true in such a process. Note that newly created process must
	 * also call enterDaemonMode to determine this state
	 */
	 
	bool inDaemonMode() const;

	///Cancels function waitForRequest asynchronous
	/**
	 *  Use this if you need to short up the timeout. 
	 */
	void cancelWaitForRequest();

	///Terminates instance
	/** Terminates the instance unconditionally. Target instance has just small
	 * chance to perform any complex action, such a cleaning of the state. You
	 * should use this function in case, that instance is not responding
	 * to the standard stop request. Function can take some time to process. After
	 * return, target instance should be terminated complete
	 *
	 * Under LINUX, two signals are sent: First SIGTERM followed by 3 seconds of waiting
	 * to give instance a chance to terminate by the standard way. In case, that
	 * instance still running, SIGKILL is sent followed by additional 3 seconds of waiting
	 *
	 * Under Windows, function TerminateProcess is called followed by 3 seconds of waiting.
	 *
	 * Function can throw TimeoutException when fails (because termination is just an signal,
	 * there is no way, how to check, whether request has been processed or will be procesed
	 * in the near future)
	 *
	 */
	void terminate();

	///Extended information current instance mode
	enum InstanceStage{
		///instance is in standard stage
		/** Standard stage, normal application, daemon mode has not been
		 * requested. You can validate service settings and load configuration
		 * in this phase. You should not allocate resources, which will
		 * be used under daemon stage.
		 *
		 * Note that on some platforms, there is an extra phase
		 * 'predaemon'. If there is such a phase, you should not load
		 * configuration in standard stage, because it cannot be propagated
		 * into predaemon phase. Test constant hasStage_predaemon to test existence of this
		 * stage on the platform
		 */
		standard,
		/// Instance is in pre-daemon stage; it is ready to enter daemon mode
		/** Predaemon mode is reported during service initialization before
		 * application enters to daemon mode. On platform without fork()
		 * support, function enterDaemonMode() restart application and
		 * enters to predaemon stage. Initialization is executed again.
		 *
		 * Simple services doesn't need to detect predaemon mode,
		 * service can validate settings and load configuration twice. It
		 * should not allocate resources in 'standard' stage, because
		 * they will be unavailable in predaemon stage.
		 *
		 * Complex services can divide validating and loading into these
		 * two stages to speed up starting.
		 *
		 * On predaemon stage, application is disconnected from original
		 * environment like daemon stage. So you should not try to use
		 * standard output.
		 */
		predaemon,
		/// Instance is in daemon stage.
		/** In daemon mode, application cannot access console, all pipes
		 * are closed and only
		 * way to communicate with environment is using instance object.
		 *
		 * Of course, application can open named pipes to reconnect
		 * pipes, if they really need.
		 *
		 */
		daemon,
		///Instance is controller.
		/** Controller is application which can post commands to
		 * daemon instance.
		 */
		controller
	};

	///Retrieves current instance stage
	InstanceStage getInstanceStage() const;

	///True, if instance can be in standard stage
	/** There is no platform, which will have this false */
	static LIGHTSPEED_EXPORT const bool hasStage_standard;
	///True, if instance can be in predaemon stage
	/** Some platforms (Windows) need this stage to enter daemon stage.
	 *  You can
	 */
	static LIGHTSPEED_EXPORT const bool hasStage_predaemon;
	///True, if instance can enter to daemon stage
	/** If there is false, platform cannot create services running
	 * on background.
	 */
	static LIGHTSPEED_EXPORT const bool hasStage_daemon;
	///True, if instance can be controller
	/** If there is false, controller will unable to detect, whether
	 * daemon is running and will unable to send messages to it.
	 */
	static LIGHTSPEED_EXPORT const bool hasStage_controller;


	class  TimeoutException: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		TimeoutException(const ProgramLocation &loc)
			:Exception(loc) {}
	protected:
		void message(ExceptionMsg &msg) const;
	};

	class  RequestTooLarge: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		RequestTooLarge(const ProgramLocation &loc)
			:Exception(loc) {}
	protected:
		void message(ExceptionMsg &msg) const;
	};

	class ReplyTooLarge: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		ReplyTooLarge(const ProgramLocation &loc)
			:Exception(loc) {}
	protected:
		void message(ExceptionMsg &msg) const;
	};

	class AlreadyRunningException: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		AlreadyRunningException(const ProgramLocation &loc, natural pid)
			:Exception(loc),pid(pid) {}

		natural getPid() const {return pid;}
	protected:
		natural pid;
		void message(ExceptionMsg &msg) const;
	};

	class  NotRunningException: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		NotRunningException(const ProgramLocation &loc)
			:Exception(loc) {}
	protected:
		void message(ExceptionMsg &msg) const;
	};

	class NotInitialized: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		NotInitialized(const ProgramLocation &loc)
			:Exception(loc) {}
	protected:
		void message(ExceptionMsg &msg) const;
	};


	class InterruptedWaitingException: public ErrNoException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		InterruptedWaitingException(const ProgramLocation &loc,
					int errnr): ErrNoException(loc,errnr) {}

	protected:
		void message(ExceptionMsg &msg) const;
	};

	void close();

protected:

	void operator=(const ProgInstance &name);

	String name;
	ProgInstanceBuffer *buffer;
	ProgInstance *prevInstance;
	
};

}
#endif /* SZN_FETCHSTATS_PROGINSTANCE_H_ */
