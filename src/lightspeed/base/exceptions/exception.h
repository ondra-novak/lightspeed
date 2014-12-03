/*
 * exception.h
 *
 *  Created on: 4.9.2009
 *      Author: ondra
 */ 

#pragma once
#ifndef _LIGHTSPEED_EXCEPTION_H_
#define _LIGHTSPEED_EXCEPTION_H_
#include "../iexception.h"
#include "../debug/programlocation.h"
#include "../memory/refCntPtr.h"
#include "exceptionMsg.h"
#include "../export.h"

namespace LightSpeed {

	class String;


    class Exception;
    typedef RefCntPtr<Exception> PException;;


    class ExceptionMsg;



    class Exception: public IException, public  RefCntObj
    {
//        typedef MemoryStreamBase<ProgramLocation> ProgramTracker;
    public:
//        typedef ProgramTracker::ReadIterator LocationIterator;


		static LIGHTSPEED_EXPORT const wchar_t *str_because;
		static LIGHTSPEED_EXPORT const wchar_t *str_exception;
		static LIGHTSPEED_EXPORT const wchar_t *str_rethrownhere;

        Exception(const ProgramLocation &loc);


        LIGHTSPEED_CLONEABLEDECL(Exception);

        ///Retrieves standard what message for std::exception
        /**
        * @return exception "what" description
        */
        virtual const char *what() const throw ();

        ///Retrieves message associated with this exception
        /** Exception should use message() to build error message
        * @return message
        * @see message
        */
        String getMessage() const;


        ///Retrieves long message that contains all informations known by exception
        /**
        * As result, exception message, type and program location is returned.
        * If exception has reason defined, it returns reason each on separate
        * line
        *
        * @return message
        */
        String getLongMessage() const;

        ///Retrieves long message that contains all informations known by exception without location
        /**
        * As result, exception message, type and program location is returned
		* It also includes reason after world "because"
        *
        * @return message
        */
        String getMessageWithReason() const;

		///Retrieves exception static description.
        /**
        * Each exception can have a static description which describes
        * reasons or conditions why exception has been thrown. Description
        * is optional, but it is recomended to specify description on
        * each different exception class
        */
        virtual const char *getStaticDesc() const;

        ///Retrieves pointer of exception attached as reason of this exception
        /**
        * Function can return NULL, if no reason has been attached.
        * If there is an exception, it can also have reason that can be
        * retrieved by this function... and so on so on.
        *
        */
        virtual const Exception *getReason() const;
        ///Retrieves program location of the exception
        /**Every exception stores location of its creation. This allows to
         * help you find reason of unexpected exception faster.
         *
         * @return Location of creation of the exception
         */
        const ProgramLocation &getLocation() const;

        ///Retrieves iterator to tracked locations
        /**
         * Tracked locations are locations, where exception has been re-thrown
         * using function rethrow() with the location parameter. This allows
         * to retrieve path during exception processing until it is catched as
         * unexpected.
         *
         * @return Iterator to process additional tracked locations
         */
//        LocationIterator getTrackedLocations() const;

        ///sets reason to the exception
        /**
         * @param rs exception used as reason. Exception is copied into
         * new instance. If reason exception contains reason,
         * that reason is shared. Every exception can have only one reason,
         * but every reason can also have reason.
         */
        void setReason(const Exception &rs);

        ///sets reason to the exception
        /**
         * @param rs exception used as reason. Exception is shared with
         * the instance. Every exception can have only one reason,
         * but every reason can also have reason.
         */
        void setReason(const PException &rs);
        ///Appends reason to the exception
        /**
		 * Adds reason to the end of reason chain. If exception has no reason
		 * function is equal to setReason
		 *
         * @param rs exception used as reason. Exception is copied into
         * new instance. If reason exception contains reason,
         * that reason is shared. Every exception can have only one reason,
         * but every reason can also have reason.
         */
        void appendReason(const Exception &rs);

        ///Appends reason to the exception
        /**
		 * Adds reason to the end of reason chain. If exception has no reason
		 * function is equal to setReason
		 *
         * @param rs exception used as reason. Exception is shared with
         * the instance. Every exception can have only one reason,
         * but every reason can also have reason.
         */
        void appendReason(const PException &rs);


        ///Rethrows the exception
        /**Useful, when you need to transfer exception to the another try...catch block
         *
         *
         * @note performs throw *this;
        */

        virtual void throwAgain() const = 0;

        ///Rethrows exception tracing down its location
        /**
         * @param loc program location where exception has been rethrown
         *
         * @note not useful in catch branch, because it creates new
         * instance of the same exception. In this situation is better
         * to call rethrow()
         *
         * @note performs addLocation(loc);throw *this;
         *
         */
        void throwAgain(const ProgramLocation &loc);


        ///tracks location and throws exception to the upper level
        /** In compare to throwAgain(), function  perform single
         * throw without parameters causing that current exception is thrown
         * to the upper level without creating new instance. Function
         * assumes, that current exception is exception processed by current
         * catch branch and adds current location into tracked location list
         *
         * @param loc current location
         *
         * @note performs addLocation(loc);throw;
         */
        void trackThrow(const ProgramLocation &loc);

		///tracks location to the exception object
		void track(const ProgramLocation &loc);

		///re-throws current exception and tracks location if current exception is Exception
		/**
	     * Function is useful in catch (...) branch, if you want to re-throw
			exception and track program location for all exceptions that
			extends Exception object. Function handles this, so no special
			support from caller is needed. Just replace empty throw with
			calling of this function.

		 * @param reference to the location. In most of cases, THISLOCATION
			constant is very useful, which will store program's current location.

		 * @return function will not return.
		 * @exception all throw current exception. 

		   @note type of current exception is detected using cascade throws
		   in internal try-catch block. For exceptions that inherits Exception
		   class, there will be one extra throw per level. Other exception
		   will be thrown only once. This can cause slightly poor performance.
		   But if exceptions is not thrown often, you will not feel any difference.

		 */
		static void rethrow(const ProgramLocation &loc);

        ///Connects exception object with the reason and returns it ready for throw
        /**
         * Use this operator to easy connect exceptions with the reason:
         *  throw MyException(THISLOCATION) << reason;
         * @param object exception object
         * @param reason object to connect as reason
         * @return function returns reference to object. Although the parameter
         * and result is marked as const, object is modified. This allows
         * to use in-line constructor for the exception object in
         * compilers, which doesn't allow to use in-line construction on
         * references without const.
         */
        template<class T>
        friend const T &operator<<(const T &object, PException reason) {
            Exception &r = const_cast<T &>(object);
            r.setReason(reason);
            return object;
        }
        template<class T>
        friend const T &operator<<(const T &object, const Exception &reason) {
            Exception &r = const_cast<T &>(object);
            r.setReason(reason);
            return object;
        }

        ///Retrieves current exception as Exception object
        /**
         * It uses rethrow pattern to convert exception. Do not use, if you
         * know the exception. Useful, when you catching three dots.
         *
         * @return pointer to exception. It returns cloned exception from
         * 	LightSpeed, or StdException if standard exception is caught, or
         *  UnknownException, if exception cannot be converted
         */
        static PException getCurrentException();

        virtual ~Exception() throw();


    protected:

		///Protected constructor allows to initialize at undefined location
		/** It is useful to construct as virtually inherited class. Note
			that inherited class still must require location and
			use setLocation() to specify location for the exception indirectly
		*/
		Exception() {}

		///Specifies location for exception
		/** Location can be specified when exception has been constructed
		using default constructor. This is case of virtually inherited class,
		where is better to use default constructor and set location in
		the body of the constructor. Note that only first call of this function
		sets the location. All other calls are ignored

		@param loc location for the exception
		@retval true location set
		@retval false skipped, location is already assigned
		*/
		bool setLocation(const ProgramLocation &loc);

        ///generates error message
        /**
         * Function request overriding and is not designed to direct call.
         * Implementation have to generate error message into the object
         * ExceptionMsg object. It also possible to call parent implementation
         * before or after the implementation.
         *
         * @param msg Object that receives exception message. Exception message
         * should be terminated by dot character as single sentence. It should
         * not contain end of line characters, except special cases. Please
         * don't put end of line character at the end of message, because
         * it can be part of another message.
         *
         * @see TextWriter
         */
        virtual void message(ExceptionMsg &msg) const = 0;

        ///Clones exception object preventing to throw any exception
        PException safeClone() const throw ();

        ///Contains message returned by what()
        mutable StdAlloc::AllocatedMemory<char> whatMsg;
        ///program location
        ProgramLocation location;
        ///Exception reason
        PException reason;
        ///contains location tracker
//        mutable ProgramTracker locationTracker;

        template<class T>
        static void rethrowSupport(const T &except) {
            throw except;
        }

    };

    #define LIGHTSPEED_EXCEPTIONFINAL \
        LIGHTSPEED_CLONEABLECLASS\
        virtual void throwAgain() const {::LightSpeed::Exception::rethrowSupport(*this);}

	#define LIGHTSPEED_EXCEPTIONFINAL_SPEC(className) \
		LIGHTSPEED_CLONEABLECLASS_SPEC(className)\
		virtual void throwAgain() const {::LightSpeed::Exception::rethrowSupport(*this);}



}
#endif /* _LIGHTSPEED_EXCEPTION_H_ */
