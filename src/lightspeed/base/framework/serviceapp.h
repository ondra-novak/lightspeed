/*
 * serviceapp.h
 *
 *  Created on: 22.2.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_FRAMEWORK_SERVICEAPP_H_
#define LIGHTSPEED_FRAMEWORK_SERVICEAPP_H_

#include "app.h"
#include "../containers/optional.h"
#include "../framework/proginstance.h"
#include "../streams/fileio.h"
#include "../sync/nulllock.h"
#include "../export.h"

namespace LightSpeed {

class  ServiceAppBase {
public:
	///Name for start command
	/** This command is used to start service. It also can be used
	 * in dispatch table to define intialization phase for the service
	 */
	static LIGHTSPEED_EXPORT const char * startCmd;
	///Name of stop command
	/** This command is used to stop service. If specified in the dispatch
	 * table, function is called before service is terminated
	 */
	static LIGHTSPEED_EXPORT const char * stopCmd;
	///Name of restart command
	/** command is used to stop current instance and start another. If
	 * specified in the dispatch table, it is called instead of start to
	 * perform special initialization for restart
	 */
	static LIGHTSPEED_EXPORT const char * restartCmd;
	///Name of start in foreground command
	/** command is used to start service at foreground (and as
	 * ordinary application under Windows). If specified in the dispatch
	 * table, it is called instead of start to perform special initialization
	 */
	static LIGHTSPEED_EXPORT const char * startForeCmd;

	///Name of wait for completion command
	/** command is used to stop execution of script until service exits.
	 * You can specify timeout as optional argument. Timeout is in milliseconds.
	 * If timeout is not specified, command will wait infinitely
	 */
	static LIGHTSPEED_EXPORT const char * waitCmd;

	///Checks service status and returns
	/**
	 * Command exits with exitCode 0 when service is running and with exitCode 1 when not
	 */
	static LIGHTSPEED_EXPORT const char * statusCmd;


	static LIGHTSPEED_EXPORT const char * testCmd;

	///keyword uses instead name of instance (as first parameter on the command line)
	/**
	 * Keyword is set to value "default". Instance name is generated depend
	 * on current platform type. Under linux, default instance name is
	 * /var/run/lightspeedsvc-XXXX where XXXX je name of application (taken
	 * from command line). You can override this by implementing method
	 * getDefaultInstanceName().
	 *
	 * Under windows, you can probably create one instance per user session.
	 *
	 */
	static LIGHTSPEED_EXPORT const wchar_t *defaultInstanceName;

	///switches to daemon mode
	/**
	 * This function initializes daemon mode. This includes
	 * need to be performed on target operation system to communicate
	 * with service manager
	 *
	 * Once application is in daemonMode, it cannot switch back to
	 * the normal mode.
	 */
	static void daemonMode();

};

class IServiceAppControl {
public:
	virtual integer postMessage(ConstStrA command, const IApp::Args & args, SeqFileOutput output) = 0;
	virtual ~IServiceAppControl() {}
};

///Helper class to write service application
/** Service application is application which can be started once and running
 * as daemon until it is stopped. You can specify instance file to
 * allow to run multiple instances.
 *
 * Every instance is able to listen commands sent from the different application which
 * opens the same instance file. In most of cases, application starts itself, if
 * command "start" is invoked. Starting another process of the same application
 * with command "stop" causes, that second process requests the first process to
 * finish its work and waits to termination
 */
class ServiceApp: public App, public ServiceAppBase, public IServiceAppControl  {
public:

	///Super class
	typedef App Super;
	///Type contains arguments
	typedef Super::Args Args;
	///Prototype of function which can be assigned to message type

	///Initializes service application
	/**
	 * @param cclass reference to class, which will handle requests
	 * @param dispatchTable pointer to dispatch table
	 * @param timeout timeout used while waiting on reply. Parameter is ignored, if
	 * 		service is started.
	 */
    ServiceApp(integer priority = 1, natural timeout = 30000);

    ///Called when there is no request pending
    /**
     * @param counter increments on every onIdle cycle starting by zero
     * @retval true continue calling onIdle
     * @retval false no mor onIdle cycles need
     *
     * Best place to change timeout by calling setTimeout. Class doesn't support
     * user timers, but this is way how to implement it.
     */
    virtual bool onIdle(natural counter);



    ///Changes timeout
    /**
     * On master process, it can be used to interrupt waiting for message and
     * process some background task. Default timeout is naturalNull, which means
     * infinite.
     *
     * On slave process, this can change timeout to waiting on reply
     *
     * @param timeout timeout in miliseconds
     */
    void setTimeout(natural timeout)
    {
        this->timeout = timeout;
    }

    ///Retrieves timeout
    /**
     *
     * @return timeout in miliseconds
     */
    natural getTimeout() const
    {
        return this->timeout;
    }

    ///Retrieves argument 0 from the command line
    /**
     * Argument 0 is unavailable in handler, this is the way, how to get argument 0
     * @return
     */
    const ConstStrW getArg0() const
    {
        return appname;
    }

    ///Sets size of buffer for request and reply.
    /**
     * @param size size of buffer in bytes. This parameter can be changed only
     * in re-implementation of function start()
     *
     */
    void setBufferSize(natural size) {
    	instanceFileSize = size;
    }
    ///Retrieves size of buffer
    /**
     * @return size of buffer in bytes. This doesn't return current size of buffer,
     * but last value used with setBufferSize()
     */
    natural getBufferSize() const {
    	return instanceFileSize;
    }

    ///Invoked when error at command line
	/**
	 @param args arguments on command line
	 @return function returns exit code. Regardless on result, program
	 is going to exit
	 */
    virtual integer onCommandLineError(const Args & args);
    ///Invoked when unknown operation
    virtual integer onOperationUnknown(ConstStrA opname);

    ///Invoked when exception caught during waiting
    /**
     * @param e caught exception.
     *
     * Exception can be result of signal posted by operation system,
     * but it also called while unhandled exception is caught from the
     * message dispatcher, or exception during onIdle()
     *
     */
    virtual void onWaitException(const Exception &) {}

    integer postMessage(ConstStrA command, const Args & args, SeqFileOutput output);

    virtual bool stop();

    ///Enables restart of service in case of error
    /**
     * Service is restarted when it encounters a critical system error, such a GPF or SIGSEGV.
     * Function installs a watchdog to watch application and relaunch it in case of error exit.
     *
     * @param r specifies count of seconds wait for next restart
     *
     * @note this function is hightly platform depend. Platform can specify additional own rules
     * for this watchdog. There is also posibility, that platform doesn't support this function.
     *
     */
    void enableRestartOnError(natural r) {restartDelaySec = r;}

    ///Restarts service due error
    /** Function can be called when service encounters a critical error and wants to restart.
     * Restart is automatically done, when system exception is thrown, such a GPF or SIGSEGV. You
     * need to call errorRestart() in other cases (in catch(...) handler)
     *
     * Function immediately terminates current application notifying the watchdog, that termination
     * has not been expected. Watchdog then launches new instance.
     *
     * Function is supported when there is also support for enableRestartOnError(). If not supported,
     * function causes exit service without restart
     */
    void errorRestart();
protected:

    ///called to process message and arguments
    /**
     * This function is always called in service process, even if command has been
     * placed in different process.
     *
     * @param command command requested by service interface
     * @param args arguments passed on command line
     * @param output stream attached to output (very limited space)
     * @return zero for success, otherwise error code
     *
     * @note Do not create threads when service starting, because this command is
     * processed before application enters into service mode. Best place to start threads
     * is startService().
     *
     *
     *
     */
    virtual integer onMessage(ConstStrA command, const Args & args, SeqFileOutput output);


    ///Called if exception during start happened.
    /**
     * Function such send error message to the stdError
     * @param output handle to error console
     * @return application's exit code
     */
    virtual integer onStartError(SeqFileOutput output);


    Optional<ProgInstance> instance;
    natural timeout;
    ConstStrW appname;
    natural instanceFileSize;
    bool stopCommand;
    natural restartDelaySec;

    virtual integer pumpMessages();
    bool processRequest(const void *request);

    ///Function is called on start of application after application enter to the service mode
    /**
     * @return Exit status, should be result of the original implementation
     *
     * @note You have call original implementation in order to continue dispatching
     * the service messages.
     */
    virtual integer startService();

	///Function is called to initialize service before it enters to service mode
	/**
	 * @param args arguments at command line, first three arguments 
     * (argv[0],argv[1],argv[2] are removed. To access application pathname
	 * use getAppPathname() function
	 *
	 * @param serr contains handle to error stream where service can put
	 *  error messages
	 * @return zero to start service, other to exit application
	 * 
	 */
	 
	virtual integer initService(const Args & , SeqFileOutput ) {return 0;}

    ///Function called on start of application before application enters to the service mode
    /**
     * You can re-implement this to perform special action in this state
     * @param args arguments from command line
     * @return exit code
     */
    integer start(const Args & args);

	///Function called on start before instance file is opened
	/** This is the right place to validate service - for example to load config
	and prepare components to initialize and start. Note that in this state,
	another service still can run, so function should not acquire resources, because
	they still can be in use. On the other hand, program can perform configuration
	check without interrupting the currently running service. In
	case of failure, currently running service is not affected 
	
	@note function is also called for command "test". If zero returned, 
	program exits with zero status.
	*/

	virtual integer validateService(const Args & , SeqFileOutput );



    ProgInstance::InstanceStage getStage() const {return instance->getInstanceStage();}




    ///Called to process command in caller.
    virtual integer startCommand(ConstStrA command, const Args & args, SeqFileOutput &serr);
private:
    void createInstanceFile();

	///Overwrite default instance name
	virtual String getDefaultInstanceName(const ConstStrW arg0) const;
};



}






#endif /* LIGHTSPEED_FRAMEWORK_SERVICEAPP_H_ */
