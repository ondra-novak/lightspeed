#include "exception.h"

#ifndef LIGHTSPEED_EXCEPTIONS_ITERATOR_H_
#define LIGHTSPEED_EXCEPTIONS_ITERATOR_H_

namespace LightSpeed {

    
    
    class NoMoreObjects: public virtual Exception {

     public:
         LIGHTSPEED_EXCEPTIONFINAL;

         NoMoreObjects(const ProgramLocation &loc, const std::type_info &objectType)
             :Exception(loc),objectType(objectType) {
         }
         NoMoreObjects(const std::type_info &objectType)
             :Exception(THISLOCATION),objectType(objectType) {}

         const std::type_info &getObjectType() const {
             return objectType;
         }

		 static LIGHTSPEED_EXPORT const char *msgText;;

	protected:

         const std::type_info &objectType;


         virtual void message(ExceptionMsg &msg) const {
        	 msg(msgText) << objectType.name();
         }

         virtual const char *getStaticDesc() const {
             return "Program encounters this exception, when it tries to work with more "
                 "objects than it is available. Exception is being used with iterators";
         }
     };

    
    class IteratorException: public virtual Exception {
    public:
        IteratorException():Exception(THISLOCATION) {}
    };

    class IteratorNoMoreItems: public NoMoreObjects, public IteratorException {
    public: 
        LIGHTSPEED_EXCEPTIONFINAL;
        IteratorNoMoreItems(const ProgramLocation &loc, const std::type_info &typeInfo):
                Exception(loc),NoMoreObjects(typeInfo) {}
    protected:
        virtual void message(ExceptionMsg &msg) const {
        	msg(msgText) << NoMoreObjects::getObjectType().name();
        }
        virtual const char *getStaticDesc() const {return NoMoreObjects::getStaticDesc();}

        static LIGHTSPEED_EXPORT const char *msgText;;
    };


    class WriteIteratorNoSpace: public NoMoreObjects, public IteratorException {
    public: LIGHTSPEED_EXCEPTIONFINAL
                WriteIteratorNoSpace(const ProgramLocation &loc, const std::type_info &typeInfo):
            Exception(loc),NoMoreObjects(typeInfo) {}
    protected:
        virtual void message(ExceptionMsg &msg) const {
        	msg(msgText) << NoMoreObjects::getObjectType().name();
        }
        virtual const char *getStaticDesc() const {return NoMoreObjects::getStaticDesc();}

        static LIGHTSPEED_EXPORT const char *msgText;;

    };

    class WriteIteratorNotAcceptable: public IteratorException {
    public: LIGHTSPEED_EXCEPTIONFINAL
			WriteIteratorNotAcceptable(const ProgramLocation &loc):
            Exception(loc) {}
    protected:
		virtual void message(ExceptionMsg &msg) const {
			msg(msgText);
        }
        virtual const char *getStaticDesc() const {
        	return "Iterator rejects the write operation, because it cannot accept the item. "
        			"To prevent this exception, you should call canAccept() function before "
        			"item is written.";
        }

        static LIGHTSPEED_EXPORT const char *msgText;;

    };

    class FilterIteratorBusyException: public IteratorException {
    public: LIGHTSPEED_EXCEPTIONFINAL
    FilterIteratorBusyException(const ProgramLocation &loc):
            Exception(loc) {}
    protected:
        virtual void message(ExceptionMsg &msg) const {
        	msg(msgText);
        }
        virtual const char *getStaticDesc() const {
        	return "In most of cases, this exception is thrown, when iterator has full internal buffer "
        			"and needs to perform some output to release some space for new items";
        }

        static LIGHTSPEED_EXPORT const char *msgText;;
    };

}

#endif /*ITERATOR_H_*/
