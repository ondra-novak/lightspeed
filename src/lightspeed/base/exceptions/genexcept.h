#pragma once
#include "./exception.h"
#include "../containers/constStr.h"

namespace LightSpeed {


	template<typename RefType>
	class GenExceptionClass: public virtual Exception {
	public:
		GenExceptionClass():Exception(THISLOCATION) {}
	};


	template<const char * &msgtext, typename Base = Exception>
	class GenException: public Base {
	public:

		GenException(const ProgramLocation &loc):Exception(loc) {}
		typedef typename Base::ICloneableBase ICloneableBase;
		LIGHTSPEED_EXCEPTIONFINAL
		
		static void setMessage(const char *newMsg) {msgtext = newMsg;}
		
		void message(ExceptionMsg &msg) const {
			msg(msgtext);
		}
		virtual ~GenException() throw() {}
	};

	template<const char * &msgtext, typename Arg1, typename Base = Exception>
	class GenException1: public Base {
	public:

		GenException1(const ProgramLocation &loc, const Arg1 &arg1)
			:Exception(loc),arg1(arg1) {}

		typedef typename Base::ICloneableBase ICloneableBase;
		LIGHTSPEED_EXCEPTIONFINAL;

		static void setMessage(const char *newMsg) {msgtext = newMsg;}
		const Arg1 &getArg1() const {return arg1;}

		void message(ExceptionMsg &msg) const {
			msg(ConstStrA(msgtext)) << arg1;
		}
		virtual ~GenException1() throw() {}
	protected:
		Arg1 arg1;
	};

	template<const char * &msgtext, typename Arg1, typename Arg2, typename Base = Exception>
	class GenException2: public Base {
	public:

		GenException2(const ProgramLocation &loc, const Arg1 &arg1, const Arg2 &arg2)
			:Exception(loc),arg1(arg1),arg2(arg2) {}

		~GenException2() throw() {}
		typedef typename Base::ICloneableBase ICloneableBase;
		LIGHTSPEED_EXCEPTIONFINAL;

		static void setMessage(const char *newMsg) {msgtext = newMsg;}
		const Arg1 &getArg1() const {return arg1;}
		const Arg2 &getArg2() const {return arg2;}

		void message(ExceptionMsg &msg) const {
			msg(msgtext) << arg1 << arg2;
		}

	protected:
		Arg1 arg1;
		Arg2 arg2;
	};

	template<const char * &msgtext, typename Arg1, typename Arg2, typename Arg3, typename Base = Exception>
	class GenException3: public Base {
	public:

		GenException3(const ProgramLocation &loc, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3)
			:Exception(loc),arg1(arg1),arg2(arg2),arg3(arg3) {}

		typedef typename Base::ICloneableBase ICloneableBase;
		LIGHTSPEED_EXCEPTIONFINAL;

		static void setMessage(const char *newMsg) {msgtext = newMsg;}
		const Arg1 &getArg1() const {return arg1;}
		const Arg2 &getArg2() const {return arg2;}
		const Arg3 &getArg3() const {return arg3;}

		void message(ExceptionMsg &msg) const {
			msg(msgtext) << arg1 << arg2 << arg3;
		}
		virtual ~GenException3() throw() {}
	protected:
		Arg1 arg1;
		Arg2 arg2;
		Arg3 arg3;
	};


	template<const char * &msgtext, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Base = Exception>
	class GenException4: public Base {
	public:

		GenException4(const ProgramLocation &loc, const Arg1 &arg1, const Arg2 &arg2, const Arg3 &arg3, const Arg4 &arg4)
			:Exception(loc),arg1(arg1),arg2(arg2),arg3(arg3),arg4(arg4) {}

		typedef typename Base::ICloneableBase ICloneableBase;
		LIGHTSPEED_EXCEPTIONFINAL;

		static void setMessage(const char *newMsg) {msgtext = newMsg;}
		const Arg1 &getArg1() const {return arg1;}
		const Arg2 &getArg2() const {return arg2;}
		const Arg3 &getArg3() const {return arg3;}
		const Arg4 &getArg4() const {return arg4;}

		void message(ExceptionMsg &msg) const {
			msg(msgtext) << arg1 << arg2 << arg3 << arg4;
		}
		virtual ~GenException4() throw() {}
	protected:
		Arg1 arg1;
		Arg2 arg2;
		Arg3 arg3;
		Arg4 arg4;
	};
}
