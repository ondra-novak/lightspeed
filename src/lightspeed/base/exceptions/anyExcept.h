#pragma  once

#include "exception.h"

namespace LightSpeed {

	class AnyTypeConversionImpossibleException: public Exception {
	public:
		LIGHTSPEED_EXCEPTIONFINAL;

		AnyTypeConversionImpossibleException(const ProgramLocation &loc,
									const std::type_info &needType, 
									const std::type_info &containType)
			: Exception(loc),needType(needType),containType(containType) {}

		///Retrieves current needType value
		/** 
		*	@return current needType value
		*/
		const std::type_info  &getNeedType() const { return needType; }

		///Retrieves current containType value
		/** 
		*	@return current containType value
		*/
		const std::type_info  &getContainType() const { return containType; }

	protected:
		const std::type_info &needType;
		const std::type_info &containType;

		void message(ExceptionMsg &msg) const {
			msg.string(L"Conversion impossible. Requested type:")
				.cString(needType.name())
				.string(L", but contains: ")
				.cString(containType.name());
		}
	};

}