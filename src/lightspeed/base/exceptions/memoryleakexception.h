
#ifndef LIGHTSPEED_EXCEPTIONS_MEMORYLEAK_H_
#define LIGHTSPEED_EXCEPTIONS_MEMORYLEAK_H_

#include "systemException.h"

namespace LightSpeed {

	class MemoryLeakException: public SystemException {
	public:

		struct Leak {
			///address of leak
			void *addr;
			///size of leak
			natural size;
			///memory block id (optional)
			natural id;

			Leak(void *addr, natural size, natural id)
				:addr(addr),size(size),id(id) {}
			Leak():addr(0),size(0),id(0) {}
		};

	    LIGHTSPEED_EXCEPTIONFINAL;

	    typedef StringCore<Leak> LeakList;

	    MemoryLeakException(const ProgramLocation &loc, const LeakList &list)
			:SystemException(loc),list(list) {
	    }

        virtual void message(ExceptionMsg &msg) const {

        	msg(msgText);
        	dumpLeaks(msg(dumpText));
        }
        const char *getStaticDesc() const {
            return "Memory leak detected";
        }


        virtual ~MemoryLeakException() throw() {}

	protected:

        static LIGHTSPEED_EXPORT const char *msgText;;
        static LIGHTSPEED_EXPORT const char *dumpText;

        template<typename T>
        void dumpLeaks(T out) const {
            for (LeakList::Iterator iter = list.getFwIter(); iter.hasItems();) {
            	const Leak &l = iter.getNext();
            	out << setBase(16) << (natural)l.addr << setBase(10) << l.size << l.id;
            	if (iter.hasItems()) out.flush();
            }
        }

	    LeakList list;
	};

}

#endif
