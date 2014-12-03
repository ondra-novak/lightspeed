#pragma once
#include "iapp.h"
#include "../memory/singleton.h"
#include "services.h"

namespace LightSpeed {


	///One of base classes for application object
	class AppBase: public IApp {
	public:

		///Defines local application
		/** Use this constant as priority to create "localApp". */
		static const integer localApp = integerNull;
		
		///Initializes base class of application
		/**
		 * @param priority specifies application priority. Priority is number
				which solves situation, when there are multiple Application 
				objects declared. Object with higher priority will become
				master application and other will become slaves. Slave
				application will not retrieve main thread, only init() and
				done() functions are called. 
				If priority is negative, application object is initialized
				as service which will not be directly executed. Method
				IApp::start() will not called. Your application must declare
				at least one object with priority zero and higher


			@see localApp
		 */
		AppBase(integer priority = 0):appPriority(priority) {
			registerApp();
		}

		///Unregisters application
		virtual ~AppBase() {
			unregisterApp();
		}
			
		///Entry for multibyte parameter line
		/**
		 * Starts this application using C-like command line. It
		 * expects command line encoded in UTF-8 code page
		 *
		 * @param argc count of parameters
		 * @param argv array of parameters
		 * @param pathname full pathname to application image
		 * @return application's exit code 
		 */
		int main_entry(int argc, char *argv[], ConstStrW pathname);

		///Entry for wide-char parameter line
		/**
         * Starts this application using C-like command line in
         * wide-char version
         *
		 * @param argc count of parameters
		 * @param argv array of parameters
		 * @param pathname full pathname to application image
		 * @return application's exit code 
		 */
		int main_entry(int argc, wchar_t *argv[], ConstStrW pathname);

		///Retrieves current application object
		/** @return reference to application with the highest priority.
		 *    Function will not return service application
		 * @exception NoCurrentApplicationException Thrown, if there
		 *    is no current application.
		 */
		static AppBase &current();

		static AppBase *currentPtr();

		///retrieves application priority
		/**@see AppBase::AppBase() */
		integer getAppPriority() const {return appPriority;}

	
		virtual void init() {
			if (prevApp != nil) prevApp->init();
		}
		virtual void done() {
			if (prevApp != nil) prevApp->done();
		}

		///Retrieves first slave application
		/**
		 * @return pointer to first slave application. Call this function
		  on slave application to retrieve next slave application, and so on.
		  You can receive NULL, if there are no more slave applications
		 */
		AppBase *getSlaveApp() const {
			return prevApp;
		}

		///Retrieves whether application has been instantiated as a service
		/**
		  @retval true object is service (priority is negative)
		  @retval false object is standard application
		 */
		bool isSerivice() const {
			return appPriority < 0;
		}


		///Retrieves pathname to current application
		/**
		 * @return disk pathname to application image file                                                                     
		 */
		ConstStrW getAppPathname() const {return appPathname;}

#ifdef _DEBUG
		virtual void onThreadException(const Exception &e) throw() {
        	exceptionDebug(e,this);
        }
#endif
	protected:
		int startApp(const Args &args);

		void handleException();





	protected:


		typedef Pointer<AppBase> AppPtr;
		typedef Singleton<AppPtr> CurAppSingleton;

		AppPtr prevApp;
		ConstStrW appPathname;
		const integer appPriority;

		void registerApp();

		void unregisterApp();

		static AppPtr getCurrentApp();

		void exceptionDebugLog(const Exception &e);
		
	};

	///The top base class for application object. You should inherit this
	/**
	 @param Lock specifies Lock object to access application services. NullLock
	 will disable locking, but you have to ensure, that service container
	 will not changed when other threads may posible read the container
	 */
	class App: public AppBase, public Services {
	public:	

        ///Initializes application
        /** @copydoc AppBase::AppBase */
        App(integer priority = 0):AppBase(priority),stopRequest(false) {
			StdAlloc::getInstance();
		}



        virtual bool stop() { stopRequest = true; return true;}

        bool needExitNow() {return stopRequest;}

		void addService(TypeInfo nfo, IInterface *ifc);
		void removeService(TypeInfo nfo);
		void removeService(TypeInfo nfo, IInterface *ifc);
		void addService(IService &ifc);
		void removeService(IService &ifc);

	protected:
	    bool stopRequest;

		typedef LinkedList<IInterface *> IfcStack;
		typedef Map<TypeInfo, IfcStack> ServiceMap;


		virtual void *proxyInterface(IInterfaceRequest &p);
		virtual const void *proxyInterface(const IInterfaceRequest &p) const;

	};


	///Thrown, when there is no current application object
	class NoCurrentApplicationException: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		NoCurrentApplicationException(const ProgramLocation &loc):Exception(loc) {}

		static LIGHTSPEED_EXPORT const char *msgText;;
	protected:
		virtual void message(ExceptionMsg &msg) const {
			msg(msgText);
		}
	};


}
