#pragma once
#include "iservices.h"
#include "../containers/arrayref.h"
#include "../containers/constStr.h"
#include "../exceptions/exception.h"

namespace LightSpeed {

	
	///Interface to access main application object
	/** To use LightSpeed application framework, you have to create
		Application object, that inherits this interface. Using
		this interface framework starts and terminates application
		*/
	class IApp: public IServices {
	public:


		typedef ConstStringT<ConstStrW> Args;

		///Replaces standard main() in LightSpeed application
		/**
		 * @param args contains arguments passed to the application
		 * @return exit code from the application
		 */
		virtual integer start(const Args &args) = 0;

		///Function is called when unhandled exception is caught
		/**
		 * @param e contains exception description. If std::exception
		 *    caught, it is converted into StdExcepton. Unknown "three dots"
		 *    exceptions are converted into UnknownException. Because
		 *    exception handler is still in effect, you can
		 *    invoke throw to rethrow the exception to catch it
		 *    in the correct exception handler
		 *
		 */
		virtual void onException(const Exception &e) {
		    exceptionDebug(e,this);
		}

		///Function is called after onException to restore application state
		/**If the function is not overwritten, function immediate
		 *    returns with exit code -1. But you can write to own version
		 *    which is able to restore application state and continue in
		 *    the work
		 * @return exit state. Because function is similar to
		 *    function start() it returns on application exit. Returned
		 *    value is used as exit code
		 */
        virtual int restore() {return 1;}


        ///Requests application to stop
        /** Function is used when application is service, shared library,
         *  module or applet. This function is requesting application to
         * stop as soon as possible.
         *
         *  Result of function don't need to be terminated application.
         *  This function is used only as signal to the application. Signal
         *  can be recorded and function leaved without any action
         *
         *  @retval true signal has been recorded and will be processed.
         *  @retval false rejected, application or user rejected this request
         */
        virtual bool stop() = 0;

        ///Initializes the event controlled application
        /**
         * Function is used to initialize event application. Event application
         * is an application, which is controlled by events. Application
         * itself is not running (active), but is started by various events.
         * Application, which an be for example a service, a shared library
         * or an applet is initialized using init() function and terminated
         * through done() function. Function init() expects, that
         * application registers itself as event handler. Registration is
         * not included into this interface, but this function is
         * presented as the very first event, which allows to perform
         * registration to the other events. Function have to exit
         * as soon as registration is done.
         *
         * When registration fails, exception should be thrown out of
         * this function
         *
         * @note to implement both application type, this method is
         * also called before start()
         */
        virtual void init() {}

        ///Exits the event controlled application
        /**
         * Function notifies application, that event environment is willing
         * to exit and need to kick out all registered event handlers. This
         * function is designed to perform these deregistrations.
         *
         * Note that there is difference between stop() and done(). Stop is
         * only signal, which can be ignored without any future problems.
         * done() cannot be ignored. After processor returns out of function
         * done, whole application should be unregistered and all
         * background thread exited and  all resources must be closed. This
         * function can be latest action processed by this application. After
         * return, object will be probably deleted - IApp has chance to
         * make some action in the destructor, but must also count with 
		 * termination without chance to do anything more.
         *
         * @note to implement both application type, this method is
         * also called after start() or restore() is finished
         */
        virtual void done() {}


        ///called when unhandled exception is caught in any slave thread
		/**
		 * @note From version lightspeed 12.5.17, threads are unable to 
		 * throw out exceptions. So this is only handler which will be
		 * called when thread causes an exception. Threads should handle
		 * and carry exceptions by own mechanism.
		 */
		 
        virtual void onThreadException(const Exception &) throw() {}

        ///used in debugging.
        /**
         * Displays content of e on standard output or error console. In
         * Windows environment, uses appropriate window to show exception
         * @param e exception to write
		 * @param app associated application - can be NULL, but when specified,
				function can use services to display more diagnostics informations
		 */
         static void exceptionDebug(const Exception &e, IApp *app);


         ///Called when exception object is created
         /**
          * @param e reference to exception being created.
          *
          * @note Referenced exception is not fully instancioned during this
          * call. You will unable to retrieve exception message at this
          * point (causing "pure virtual call error"!). You can access
          * exception's location and exception's reason object. You can
          * also store address of the exception for diagnostics purpose, but
          * remember, that address of caught exception becomes invalid
          *
          * It is possible to store pointer in case custom terminate handler
          * is called
          */
         virtual void onFirstChanceException(const Exception *) throw() {}

		 ///retrieves current application
		 static IApp &current();		 

		 static IApp *currentPtr();	

		 static void threadException(const Exception &e) throw() {
			 IApp *a = currentPtr();
			 if (a) a->onThreadException(e);
			 else std::unexpected();
		 }


		 virtual ~IApp() {}


	};


	
}
