#ifndef _LIGHTSPEED_MESSAGES_FUNCTIONCALL_
#define _LIGHTSPEED_MESSAGES_FUNCTIONCALL_
#include "../qualifier.h"

namespace LightSpeed {

	template<typename Obj, typename Fn, typename RetT>
	struct MemberFunctionAsFunctor {

		Obj obj;
		Fn fn;

		MemberFunctionAsFunctor(typename FastParam<Obj>::T obj, Fn fn)
			:obj(obj),fn(fn) {}

		RetT operator()() const {return (RetT)(obj->*fn)();}
		template<typename X>
		RetT operator()(const X &x) const {return (RetT)(obj->*fn)(x);}
		template<typename X, typename Par1>
		RetT operator()(const X &x, const Par1 &par1) const {return (RetT)(obj->*fn)(x,par1);}
		template<typename X, typename Par1, typename Par2>
		RetT operator()(const X &x, const Par1 &par1, const Par2 &par2) const {return (RetT)(obj->*fn)(x,par1,par2);}
	};


	template<typename Fn, typename RetT>
	struct MemberFunctionCall {

		Fn fn;

		MemberFunctionCall(Fn fn):fn(fn) {}

		template<typename Obj>
		RetT operator()(const Obj &obj) const {return (RetT)(obj->*fn)();}
		template<typename Obj, typename Par1>
		RetT operator()(const Obj &obj, const Par1 &par1) const {return (RetT)(obj->*fn)(par1);}
		template<typename Obj, typename Par1, typename Par2>
		RetT operator()(const Obj &obj, const Par1 &par1, const Par2 &par2) const {return (RetT)(obj->*fn)(par1,par2);}
	};


	template<typename Fn>
	struct FunctionCall {
		typedef Fn T;
	};



	template<typename A, typename B>
	struct FunctionCall<A (B::*)() > {
		typedef MemberFunctionCall<A (B::*)(),A> T;
	};

	template<typename A, typename B>
	struct FunctionCall<A (B::*)() const > {
		typedef MemberFunctionCall<A (B::*)() const,A> T;
	};

	template<typename A, typename B, typename C>
	struct FunctionCall<A (B::*)(C) > {
		typedef MemberFunctionCall<A (B::*)(C),A> T;
	};

	template<typename A, typename B, typename C>
	struct FunctionCall<A (B::*)(C) const > {
		typedef MemberFunctionCall<A (B::*)(C) const,A> T;
	};

	template<typename A, typename B, typename C, typename D>
	struct FunctionCall<A (B::*)(C,D) > {
		typedef MemberFunctionCall<A (B::*)(C,D),A> T;
	};

	template<typename A, typename B, typename C,typename D>
	struct FunctionCall<A (B::*)(C,D) const > {
		typedef MemberFunctionCall<A (B::*)(C,D) const,A> T;
	};
}


#endif
