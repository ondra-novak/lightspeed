#include "exception.h"
#ifndef LIGHTSPEED_EXCEPTIONS_OUTOFMEMORY_H_
#define LIGHTSPEED_EXCEPTIONS_OUTOFMEMORY_H_

#include "systemException.h"
#include "errorMessageException.h"

namespace LightSpeed {

	class OutOfMemoryException: public SystemException {
	public:
		OutOfMemoryException(const ProgramLocation &loc, natural reqMemory = 0)
			:Exception(loc), SystemException(loc),reqMemory(reqMemory) {}
		
		LIGHTSPEED_EXCEPTIONFINAL;
		
		
		natural getRequestedMemorySize() const {
			return reqMemory;
		}
		
	protected:
		natural reqMemory;
        virtual void message(ExceptionMsg &msg) const {
        	msg(msgText) << reqMemory;
		}

        static LIGHTSPEED_EXPORT const char *msgText;;
	};

	class AllocatorLimitException: public Exception {
	public:
	    LIGHTSPEED_EXCEPTIONFINAL;

	    AllocatorLimitException(const ProgramLocation &loc, natural reqCount,
	        natural avalableCount, const std::type_info &allocatorType)
                :Exception(loc),reqCount(reqCount),
                availableCount(avalableCount),allocatorType(allocatorType) {}

        virtual void message(ExceptionMsg &msg) const {
        	msg(msgText) << allocatorType.name() << reqCount << availableCount;
        }
        const char *getStaticDesc() const {
            return "Exception can be thrown by various allocators, when "
                    "they cannot extends allocated memory due allocator limit";
        }


	protected:
        natural reqCount;
	    natural availableCount;
	    const std::type_info &allocatorType;

        static LIGHTSPEED_EXPORT const char *msgText;;

	};

		
} // namespace LightSpeed


#endif /*OUTOFMEMORY_H_*/
