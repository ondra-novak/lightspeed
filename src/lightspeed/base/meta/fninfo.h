#ifndef _LIGHTSPEEED_META_FUNCTIONINFO
#define _LIGHTSPEEED_META_FUNCTIONINFO

#pragma once

namespace LightSpeed {


	template<typename Fn> struct FunctionInfo; //Not defined



	template<typename R> struct FunctionInfo<R (*)()> {
		static const natural paramCount = 0;
		typedef R Ret;
	};

	template<typename R, typename P1> struct FunctionInfo<R (*)(P1)> {
		static const natural paramCount = 1;
		typedef R Ret;
		typedef P1 Param1;
	};
	template<typename R, typename P1, typename P2>
	struct FunctionInfo<R (*)(P1,P2)> {
		static const natural paramCount = 2;
		typedef R Ret;
		typedef P1 Param1;
		typedef P2 Param2;
	};
	template<typename R, typename P1, typename P2, typename P3>
	struct FunctionInfo<R (*)(P1,P2,P3)> {
		static const natural paramCount = 3;
		typedef R Ret;
		typedef P1 Param1;
		typedef P2 Param2;
		typedef P3 Param3;
	};
	template<typename R, typename P1, typename P2, typename P3, typename P4>
	struct FunctionInfo<R (*)(P1,P2,P3,P4)> {
		static const natural paramCount = 4;
		typedef R Ret;
		typedef P1 Param1;
		typedef P2 Param2;
		typedef P3 Param3;
		typedef P4 Param4;
	};

	template<typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
	struct FunctionInfo<R (*)(P1,P2,P3,P4,P5)> {
		static const natural paramCount = 5;
		typedef R Ret;
		typedef P1 Param1;
		typedef P2 Param2;
		typedef P3 Param3;
		typedef P4 Param4;
		typedef P5 Param5;
	};


	template<typename O, typename R> struct FunctionInfo<R (O::*)()> {
		static const natural paramCount = 0;
		typedef R Ret;
		typedef O Obj;
	};

	template<typename O, typename R, typename P1> struct FunctionInfo<R (O::*)(P1)> {
		static const natural paramCount = 1;
		typedef R Ret;
		typedef O Obj;
		typedef P1 Param1;
	};
	template<typename O, typename R, typename P1, typename P2>
	struct FunctionInfo<R (O::*)(P1,P2)> {
		static const natural paramCount = 2;
		typedef R Ret;
		typedef O Obj;
		typedef P1 Param1;
		typedef P2 Param2;
	};
	template<typename O, typename R, typename P1, typename P2, typename P3>
	struct FunctionInfo<R (O::*)(P1,P2,P3)> {
		static const natural paramCount = 3;
		typedef R Ret;
		typedef O Obj;
		typedef P1 Param1;
		typedef P2 Param2;
		typedef P3 Param3;
	};
	template<typename O, typename R, typename P1, typename P2, typename P3, typename P4>
	struct FunctionInfo<R (O::*)(P1,P2,P3,P4)> {
		static const natural paramCount = 4;
		typedef R Ret;
		typedef O Obj;
		typedef P1 Param1;
		typedef P2 Param2;
		typedef P3 Param3;
		typedef P4 Param4;
	};

	template<typename O, typename R, typename P1, typename P2, typename P3, typename P4, typename P5>
	struct FunctionInfo<R (O::*)(P1,P2,P3,P4,P5)> {
		static const natural paramCount = 5;
		typedef R Ret;
		typedef O Obj;
		typedef P1 Param1;
		typedef P2 Param2;
		typedef P3 Param3;
		typedef P4 Param4;
		typedef P5 Param5;
	};

}  // namespace LightSpeed

#endif

