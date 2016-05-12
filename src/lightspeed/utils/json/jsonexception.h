#pragma once

#include "../../base/exceptions/exception.h"
namespace LightSpeed {

	namespace JSON {

		class RequiredFieldException: public Exception {
		public:
			LIGHTSPEED_EXCEPTIONFINAL;
			RequiredFieldException(const ProgramLocation &loc, String fieldName)
				: Exception(loc),fieldName(fieldName) {}
			~RequiredFieldException() throw() {}

			const String &getFieldName() const {return fieldName;}
		protected:
			String fieldName;

			void message(ExceptionMsg &msg) const;
		};



		class ParseError_t: public Exception {
		public:
			LIGHTSPEED_EXCEPTIONFINAL;

			ParseError_t(const ProgramLocation &loc, String nearStr):
				Exception(loc),nearStr(nearStr) {}
			~ParseError_t() throw() {}
			const String &getNearStr() const {return nearStr;}
		protected:
			String nearStr;

			virtual void message(ExceptionMsg &msg) const;

		};

		class SharedValueException: public Exception {
		public:
			LIGHTSPEED_EXCEPTIONFINAL;

			static const char *msgText;
			SharedValueException(const ProgramLocation &loc):Exception(loc) {}
		protected:
			void message(ExceptionMsg &msg) const;
		};


	}
}
