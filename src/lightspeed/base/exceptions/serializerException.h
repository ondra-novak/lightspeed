 #ifndef LIGHTSPEED_SERIALIZER_EXCEPTIONS_H_
#define LIGHTSPEED_SERIALIZER_EXCEPTIONS_H_

#include "exception.h"
#include "../framework/iservices.h"

namespace LightSpeed
{
    class SerializerException: public virtual Exception {
    public:
        SerializerException(const ProgramLocation &loc) {
			setLocation(loc);
		}
    };

    
	class SerializerNoSectionActiveException: public SerializerException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		SerializerNoSectionActiveException(const ProgramLocation &loc,
			const std::type_info &nfo):SerializerException(loc),nfo(nfo) {}

		const std::type_info &getSerializerType();

		static LIGHTSPEED_EXPORT const char *msgText;;
	protected:
		const std::type_info &nfo;

		virtual void message(ExceptionMsg &msg) const {
			msg(msgText) << nfo.name();
		}
 
	};


	class ClassNotRegisteredException: public SerializerException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		ClassNotRegisteredException(const ProgramLocation &loc, const std::type_info &typeInfo)
			:SerializerException(loc),typeInfo(typeInfo) {}


		const std::type_info &getClassInfo() const {return typeInfo;}

		static LIGHTSPEED_EXPORT const char *msgText;;

	protected:
		const std::type_info &typeInfo;

		void message(ExceptionMsg &msg) const {
			msg(msgText) << typeInfo.name();
		}
	};

	class UnknownObjectIdentifierException: public SerializerException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		UnknownObjectIdentifierException(const ProgramLocation &loc, const StringA &name)
			:SerializerException(loc),name(name) {}


		const StringA &getClassInfo() const {return name;}

		static LIGHTSPEED_EXPORT const char *msgText;;

		virtual ~UnknownObjectIdentifierException() throw() {}

	protected:
		StringA name;

		void message(ExceptionMsg &msg) const {
			msg(msgText) << name;
		}
	};


	class SubsectionException: public SerializerException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		SubsectionException(const ProgramLocation &loc,
			const StringA &section)
			:SerializerException(loc),section(section) {}
		const StringA &getSectionName() const {return section;}
		static LIGHTSPEED_EXPORT const char *msgText;;

		virtual ~SubsectionException() throw() {}
	protected:
		StringA section;
		void message(ExceptionMsg &msg) const {
			msg(msgText) << section;
		}
	};

	class RequiredSectionException: public SubsectionException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		RequiredSectionException(const ProgramLocation &loc,
			const StringA &section)
			:SubsectionException(loc,section) {}
		static LIGHTSPEED_EXPORT const char *msgText;;
	protected:
		void message(ExceptionMsg &msg) const {
			msg(msgText) << section;
		}
	};

	class UnexpectedSectionException: public SubsectionException {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;
		UnexpectedSectionException(const ProgramLocation &loc,
			const StringA &section)
			:SubsectionException(loc,section) {}

		static LIGHTSPEED_EXPORT const char *msgText;;
	protected:
		void message(ExceptionMsg &msg) const {
			msg(msgText) << section;
		}
	};
} // namespace LightSpeed



#endif /*EXCEPTIONS_H_*/
