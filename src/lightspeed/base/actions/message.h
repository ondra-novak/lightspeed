/*
 * message.h
 *
 *  Created on: 21.8.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MESSAGES_MESSAGE_H_
#define LIGHTSPEED_MESSAGES_MESSAGE_H_


#include "argument.h"
#include "functionCall.h"
#include "../memory/runtimeAlloc.h"
#include "../../base/memory/cloneable.h"
#include "../memory/ownedPointer.h"
#include "../memory/refCntPtr.h"
#include "../memory/stdAlloc.h"
#include "../meta/isConvertible.h"
namespace LightSpeed {


	///Message object
	/** Message in LightSpeed library is always a function call invoked in the time of delivering.
	 * To deliver a message means to call function carried by the message
	 *
	 * Message can carry a lamba function, static function, member function of predefined
	 * object or member function of object that will be defined on time of delivery.
	 *
	 * To use message, one have to typedef Message template and define its arguments
	 *
	 * @tparam RetT specifies return value of the function call. Every message can return a value
	 *   as its direct reply. If you don't need return value, specify 'void'
	 *
	 * @tparam TargetT specifies type or class of target. This is primarily useful for messages
	 * containing reference to member function of this class. On time of delivery, caller must
	 * supply an object as target and member fuction is called with that object. If message
	 * doesn't carry member function, target is supplied as first argument of the function
	 *
	 * @tparam SourceT specifies type or class of source. Again like TargetT, this is primaarily
	 * useful for messages containing reference to member function. It should
	 * contain information about source and it is passed as argument of that function. In case
	 * thah message doesn't carry member function, source is passed as second argument. In case
	 * thah TargetT is 'void', SourceT is always passed as first argument of the function. That
	 * message can't carry member function.
	 *
	 *
	 * Additionally, message itself can carry one argument supplied by the creator of the message so
	 * final function can have up to 3 arguments.
	 *
	 * Message<X,void,void> - single function: zero or one arguments,
	 *                        member function with object: zero or one arguments,
	 *                        member function without object: --error--
	 *
	 *
	 * Message<X,TargetT,void> - single function: one or two arguments,
	 *                           member function with object: one or two arguments,
	 *                           member function without object: zero or one arument (target is this)
	 *
	 * Message<X,void,SourceT> - single function: one or two arguments,
	 *                           member function with object: one or two arguments,
	 *                           member function without object: --error--
	 *
	 * Message<X,TargetT,SourceT> - single function: two or three arguments,
	 *                           member function with object: two or three arguments,
	 *                           member function without object: one or two
	 *
	 *  Arguments are always passed in following order
	 *
	 *  fn(target, source, user);
	 *
	 *  target->fn(source, user);
	 *
	 *
	 *  @note TargetT is expected to be pointer or smart pointer. In general you should avoid
	 *  using references for any argument.
	 *
	 *
	 *  @note changes after 14.09:
	 *
	 *  This class should no longer be used in arguments as reference. If you want
	 *  to continue to use create() function to build message directly for delivering
	 *  you can use Message::Ifc, which is typedef to apropriate IMessage reference.
	 *
	 *  Functions Message::create still helps to build message object like they used to prior 14.09
	 *
	 *  Using Message asi argument causes, that built message is copied into dynamically allocated object
	 *  and used to initialize hidden Message object. This can be slower.
	 *
	 *  You no longer need to create pointers to message, because Message object works as pointer.
	 *  Instances are ref counted, they can be copied without performance lost. Ref counting
	 *  is MT safe
	 *
	 *  How to use object now
	 *
	 *  Requiring and creating message on stack (fast)
	 *
	 *  @code
	 *  void delivery(const Message<void>::Ifc &msg) {
	 *    msg();
	 *  }
	 *
	 *  int main()
	 *  {
	 *    delivery(Message<void>::create(&foo, "bar"));
	 *    // delivery(Message<void>(&foo, "bar")); - works too but it is slower
	 *  }
	 *  @endcode
	 *
	 *  Creating message on heap (standard)
	 *
	 *  @code
	 *  void delivery(Message<void> msg) {
	 *    msg.deliver(); //msg() works too
	 *  }
	 *
	 *  int main()
	 *  {
	 *   // delivery(Message<void>::create(&foo, "bar")); - works too
	 *    delivery(Message<void>(&foo, "bar"));
	 *  }
	 *  @endcode
	 *
	 *  Using lambda functions
	 *
	 *  @code
	 *  Message<void>([] { ... });  //create Message object
	 *  Message<void>::create([] { ... }); //create IMessage instance
	 *  @endcode
	 *
	 *  You also can
	 *
	 *  @code
	 *  Use const IMessage & where Message<> is expected - cloned
	 *  Use const IMessage * where Message<> is expected - shared
	 *  Use Message<> where IMessage is expected - cast operator
	 *  @endcode
	 *
	 *
	 */
	template <typename RetT=void, typename TargetT=void, typename SourceT=void>
	class Message;


	template <typename RetT=void, typename TargetT=void, typename SourceT=void>
	class IMessage;
	template<typename Fn, typename RetT, typename TargetT, typename SourceT>
	class MessageImpl;
	template<typename Fn, typename RetT, typename TargetT, typename SourceT>
	class MessageImpl1;
	template<typename Fn, typename Args, typename RetT, typename TargetT, typename SourceT>
	class MessageImpl2;
	template<typename Obj, typename Fn, typename Args, typename RetT, typename TargetT, typename SourceT>
	class MessageImpl3;

	template<> struct Argument<void> {typedef bool T;};


	///Returns allocator used to clone messages
	/** Default allocator is PoolAlloc. It allows to allocate object fast and it is MT safe.*/
	IRuntimeAlloc &getMessageDefaultAllocator();
	///Changes default allocator for messages
	void setMessageDefaultAllocator( IRuntimeAlloc &alloc );


	template<typename RetT , typename TargetT , typename SourceT >
	class MessageBase {
	public:

		///Contains type for interface to access this message directly
		typedef IMessage<RetT,TargetT,SourceT> Ifc;
		typedef const Ifc &Ref;
		typedef RefCntPtr<Ifc> PMessage;

		///Constructs empty object
		/** Don't use this unless you plan to supply message later.
		 *
		 */
		MessageBase();

		MessageBase(const Ifc &a):msg(a.clone()) {msg = msg.getMT();}

		///Universal constructor
		/**
		 * @param a depend on type of arg
		 *
		 * if type of a is pointer to instance of IMessage, object is initialized claiming reference to this message
		 *
		 * If type if a is const reference to instance of IMessage, object makes copy of message and use copy for initialization
		 *
		 * For all other types, constructor expects function, lambda function, or function object (it have to implement operator ()).
		 * Constructor uses object to construct new message and this message is used to object initialization
		 */
		template<typename A>
		MessageBase(const A &a):msg(initProxy(a, typename MIsConvertible<A, const Ifc *>::MValue())) {msg = msg.getMT();}
		///Constructs object using two arguments
		/**
		 * There are two ways, how to construct this object
		 *
		 * Var 1: create function call with user argument
		 *
		 * @param a function, lambda function, function object
		 * @param b user defined argument passed as last argument to the function
		 *
		 * Var 2: create member function call on specified object
		 *
		 * @param a pointer or smart pointer to an object
		 * @param b pointer to member function of that object with apropriate count of arguments depend on message type
		 */
		template<typename A, typename B>
		MessageBase(const A &a, const B &b):msg(new(getMessageDefaultAllocator()) MessageImpl2<A,B,RetT,TargetT,SourceT>(a,b)) {msg = msg.getMT();}

		///Constructs object using three arguments
		/** Currently only one way how to use
		 *
		 * @param a pointer or smart pointer to an object
		 * @param b pointer to member function of that object with apropriate count of arguments depend on message type
		 * @param c user defined argument passed as last argument to the function
		 */
		template<typename A, typename B, typename C>
		MessageBase(const A &a, const B &b, const B &c):msg(new(getMessageDefaultAllocator()) MessageImpl3<A,B,C,RetT,TargetT,SourceT>(a,b,c)) {msg = msg.getMT();}

		///Creates message using the functor or lambda
		/**
		 * @param fn instance of functor or lambda. It can also by method of an object.
		 *  In that case, TargetT must be pointer to the object.
		 *
		 * @return instance of message which contains functor or lambda and
		 * which is executed when message is delivered.
		 *
		 */

		template<typename Fn>
		static MessageImpl1<Fn,RetT,TargetT,SourceT> create(const Fn &fn) {
			return MessageImpl1<Fn,RetT,TargetT,SourceT>(fn);
		}

		///Creates function call with argument or member function call without argument
		/**
\		 * @param a function or functor or pointer to an object. Function chooses implementation depend on type
		 * @param b if a is function, b is extra argument. If a is pointer to object, b is pointer to member function.
		 * @return Message object
		 */
		template<typename A, typename B>
		static MessageImpl2<A,B,RetT,TargetT,SourceT> create(const A &a, const B &b) {
			return MessageImpl2<A,B,RetT,TargetT,SourceT>(a,b);
		}

		///Create member function call
		/**
		 * @param obj pointer to object (or smart pointer to object)
		 * @param fn pointer to member function
		 * @param args arguments passed with message
		 * @return message object
		 */
		template<typename Obj, typename Fn, typename Args>
		static MessageImpl3<Obj, Fn,Args,RetT,TargetT,SourceT> create(const Obj &obj, const Fn &fn, const Args &args) {
			return MessageImpl3<Obj, Fn,Args,RetT,TargetT,SourceT>(obj, fn,args);
		}

		///Creates copy of message
		/**
		 * @return pointer to Ifc containing copied message.
		 */
		Ifc *clone(IRuntimeAlloc &alloc) const {return msg->clone(alloc);}

		Ifc *clone() const {return msg->clone(StdAlloc::getInstance());}

		const Ifc &ifc() const {return *msg;}

		operator Ifc &() {return *msg;}
		operator const Ifc &() const {return *msg;}

	protected:

		PMessage msg;

		template<typename Fn>
		static PMessage initProxy(const Fn &fn, MFalse) {
			return initProxy2(fn,typename MIsConvertible<Fn,const Ifc &>::MValue());
		}

		static PMessage initProxy(const Ifc *ptr, MTrue) {
			return const_cast<Ifc *>(ptr);
		}
		template<typename Fn>
		static PMessage initProxy2(const Fn &fn, MFalse) {
			return new(getMessageDefaultAllocator()) MessageImpl1<Fn,RetT,TargetT,SourceT>(fn);
		}
		static PMessage initProxy2(const Ifc &ptr, MTrue) {
			return ptr.clone();
		}

	};

	template<typename RetT , typename TargetT , typename SourceT >
	class Message: public MessageBase<RetT, TargetT, SourceT> {
	public:

		typedef MessageBase<RetT, TargetT, SourceT> Super;
		typedef typename Super::Ifc Ifc;
		typedef typename Super::Ref Ref;
		typedef typename Super::PMessage PMessage;

		Message();
		template<typename Fn>
		Message(const Fn &fn):Super(fn) {}
		template<typename A, typename B>
		Message(const A &a, const B &b):Super(a,b) {}
		template<typename A, typename B, typename C>
		Message(const A &a, const B &b, const B &c):Super(a,b,c) {}
		Message(const Message &other):Super(other) {}


		///deliver message with two argument
		RetT operator()(typename Ifc::Arg1 a1, typename Ifc::Arg2 a2) const {return (RetT)(*Super::msg)(a1,a2);}

		///deliver message with two argument
		RetT deliver(typename Ifc::Arg1 a1, typename Ifc::Arg2 a2) const {return (RetT)(*Super::msg)(a1,a2);}
	};


	template<typename RetT  , typename TargetT >
	class Message<RetT, TargetT, void>: public MessageBase<RetT, TargetT, void> {
	public:

		typedef MessageBase<RetT, TargetT, void > Super;
		typedef typename Super::Ifc Ifc;
		typedef typename Super::Ref Ref;
		typedef typename Super::PMessage PMessage;

		Message();
		template<typename Fn>
		Message(const Fn &fn):Super(fn) {}
		template<typename A, typename B>
		Message(const A &a, const B &b):Super(a,b) {}
		template<typename A, typename B, typename C>
		Message(const A &a, const B &b, const B &c):Super(a,b,c) {}
		Message(const Message &other):Super(other) {}

		RetT operator()(typename Ifc::Arg1 a1) const {return (RetT)(*Super::msg)(a1);}
		RetT deliver(typename Ifc::Arg1 a1) const {return (RetT)(*Super::msg)(a1);}
	};

	template<typename RetT  , typename SourceT >
	class Message<RetT, void, SourceT>: public MessageBase<RetT, void, SourceT> {
	public:

		typedef MessageBase<RetT, void, SourceT> Super;
		typedef typename Super::Ifc Ifc;
		typedef typename Super::Ref Ref;
		typedef typename Super::PMessage PMessage;

		template<typename Fn>
		Message(const Fn &fn):Super(fn) {}
		template<typename A, typename B>
		Message(const A &a, const B &b):Super(a,b) {}
		template<typename A, typename B, typename C>
		Message(const A &a, const B &b, const B &c):Super(a,b,c) {}
		Message(const Message &other):Super(other) {}

		RetT operator()(typename Ifc::Arg1 a1) const {return (RetT)(*Super::msg)(a1);}
		RetT deliver(typename Ifc::Arg1 a1) const {return (RetT)(*Super::msg)(a1);}
	};

	template<typename RetT >
	class Message<RetT, void, void>: public MessageBase<RetT, void, void> {
	public:
		typedef MessageBase<RetT, void, void> Super;
		typedef typename Super::Ifc Ifc;
		typedef typename Super::Ref Ref;
		typedef typename Super::PMessage PMessage;

		Message();
		template<typename Fn>
		explicit Message(const Fn &fn):Super(fn) {}
		template<typename A, typename B>
		Message(const A &a, const B &b):Super(a,b) {}
		template<typename A, typename B, typename C>
		Message(const A &a, const B &b, const B &c):Super(a,b,c) {}
		Message(const Message &other):Super(other) {}

		RetT operator()() const {return (RetT)(*Super::msg)();}
		RetT deliver() const {return (RetT)(*Super::msg)();}
	};

	///Action is alias for Message with doesn't require arguments and doesn't return anything
	typedef Message<void> Action;
	///IAction is alias for IMessage with doesn't require arguments and doesn't return anything
	typedef Message<void>::Ifc IAction;



	///interface of the message supports target and source
	/** @see Message */
	template<typename RetT, typename TargetT, typename SourceT>
	class IMessage: public ICloneable, public RefCntObj, public DynObject {
	public:
		typedef typename Argument<TargetT>::T Arg1;
		typedef typename Argument<SourceT>::T Arg2;


		virtual RetT operator()(Arg1 ctx, Arg2 invoker) const = 0;

		template<typename Fn>
		static MessageImpl<Fn,RetT,TargetT,SourceT> create(const Fn &fn);


		LIGHTSPEED_CLONEABLEDECL(IMessage)

		virtual ~IMessage() {}

	};

	///interface of the message supports target
	/** @see Message */
	template<typename RetT, typename TargetT>
	class IMessage<RetT, TargetT,void>: public ICloneable, public RefCntObj, public DynObject {
	public:

		typedef typename Argument<TargetT>::T Arg1;
		typedef typename Argument<void>::T Arg2;

		virtual RetT operator()(Arg1 ctx) const = 0;

		template<typename Fn>
		static MessageImpl<Fn,RetT,TargetT,void> create(const Fn &fn);

		LIGHTSPEED_CLONEABLEDECL(IMessage)

		virtual ~IMessage() {}

	};

	///interface of the message supports target
	/** @see Message */
	template<typename RetT, typename SourceT>
	class IMessage<RetT, void, SourceT>: public ICloneable, public RefCntObj, public DynObject {
	public:

		typedef typename Argument<SourceT>::T Arg1;
		typedef typename Argument<void>::T Arg2;

		virtual RetT operator()(Arg1 ctx) const = 0;

		template<typename Fn>
		static MessageImpl<Fn,RetT,void, SourceT> create(const Fn &fn);

		LIGHTSPEED_CLONEABLEDECL(IMessage)

		virtual ~IMessage() {}

	};

	template<typename RetT>
	class IMessage<RetT,void,void>: public ICloneable, public RefCntObj, public DynObject {
	public:
		typedef typename Argument<void>::T Arg1;
		typedef typename Argument<void>::T Arg2;

		virtual RetT operator()() const = 0;

		template<typename Fn>
		static MessageImpl<Fn,RetT,void,void> create(const Fn &fn);

		LIGHTSPEED_CLONEABLEDECL(IMessage)

		virtual ~IMessage() {}

	};

	template<typename Fn, typename RetT>
	class MessageImpl<Fn,RetT,void,void>: public IMessage<RetT,void,void> {
	public:
		virtual RetT operator()() const {
			return (RetT)fn();
		}

		typedef MessageImpl ICloneableBase;
		LIGHTSPEED_CLONEABLECLASS

		virtual ~MessageImpl() {}

		MessageImpl(const Fn &fn):fn(fn) {}
	protected:
		Fn fn;

	};


	template<typename Fn, typename RetT, typename TargetT>
	class MessageImpl<Fn,RetT,TargetT,void>: public IMessage<RetT,TargetT,void> {
	public:
		typedef IMessage<RetT,TargetT,void> Super;


		MessageImpl(Fn fn):fn(fn) {}
		virtual RetT operator()(typename Super::Arg1 ctx) const {
			return (RetT)fn(ctx);
		}

		virtual ~MessageImpl() {}

		typedef MessageImpl ICloneableBase;
		LIGHTSPEED_CLONEABLECLASS




	protected:
		Fn fn;
	};

	template<typename Fn, typename RetT, typename SourceT>
	class MessageImpl<Fn,RetT,void,SourceT>: public IMessage<RetT,void,SourceT> {
	public:
		typedef IMessage<RetT,void,SourceT> Super;

		MessageImpl(Fn fn):fn(fn) {}
		virtual RetT operator()(typename Super::Arg1 ctx) const {
			return (RetT)fn(ctx);
		}

		typedef MessageImpl ICloneableBase;
		LIGHTSPEED_CLONEABLECLASS

		virtual ~MessageImpl() {}

	protected:
		Fn fn;
	};

	template<typename Fn, typename RetT, typename TargetT, typename SourceT>
	class MessageImpl: public IMessage<RetT,TargetT,SourceT> {
	public:
		typedef IMessage<RetT,TargetT,SourceT> Super;

		MessageImpl(Fn fn):fn(fn) {}

		virtual RetT operator()(typename Super::Arg1 ctx, typename Super::Arg2 invoker) const {
			return (RetT)fn(ctx,invoker);
		}

		typedef MessageImpl ICloneableBase;
		LIGHTSPEED_CLONEABLECLASS

		virtual ~MessageImpl() {}


	protected:
		Fn fn;
	};

	template<typename RetT, typename Fn, typename Args>
	class FuncWithArgs {
	public:

		FuncWithArgs(Fn fn, typename Argument<Args>::T args)
			:fn(fn),args(args) {}

		RetT operator()() const {return (RetT)fn(args);}
		template<typename A>
		RetT operator()(const A &a) const {return (RetT)fn(a, args);}
		template<typename A, typename B>
		RetT operator()(const A &a, const B &b) const {return (RetT)fn(a,b, args);}


	protected:
		Fn fn;
		Args args;
	};

	template<typename A, typename B>
	struct FuncTwoArgs {
		A arg1;
		B arg2;
	};

	template<typename RetT, typename Fn, typename AA, typename AB>
	class FuncWithArgs<RetT,Fn,FuncTwoArgs<AA, AB> > {
	public:

		FuncWithArgs(Fn fn, const FuncTwoArgs<AA, AB> &args)
			:fn(fn),args(args) {}

		RetT operator()() const {return (RetT)fn(args);}
		template<typename A>
		RetT operator()(const A &a) const {return (RetT)fn(a, args);}
		template<typename A, typename B>
		RetT operator()(const A &a, const B &b) const {return (RetT)fn(a,b, args);}


	protected:
		Fn fn;
		FuncTwoArgs<AA,AB> args;
	};

	template<typename Fn, typename RetT, typename TargetT, typename SourceT>
	class MessageImpl1: public MessageImpl<typename FunctionCall<Fn>::T,RetT,TargetT,SourceT > {
		typedef MessageImpl<typename FunctionCall<Fn>::T,RetT,TargetT,SourceT > Super;
	public:
		MessageImpl1(Fn fn):Super(fn) {}
	};

	template<typename Fn, typename Args, typename RetT, typename TargetT, typename SourceT>
	class MessageImpl2Fn: public MessageImpl<
								FuncWithArgs<RetT, typename FunctionCall<Fn>::T,Args>,
								RetT,TargetT,SourceT > {

		typedef FuncWithArgs<RetT, typename FunctionCall<Fn>::T,Args> FnArgs;
		typedef MessageImpl<FnArgs,RetT,TargetT,SourceT > Super;
	public:
		MessageImpl2Fn(Fn fn, typename Argument<Args>::T args):Super(FnArgs(fn,args)) {}
	};

	template<typename Obj, typename Fn, typename RetT, typename TargetT, typename SourceT>
	class MessageImpl2Mbr: public MessageImpl<
								MemberFunctionAsFunctor<Obj,Fn,RetT>,
								RetT,TargetT,SourceT > {

		typedef MemberFunctionAsFunctor<Obj,Fn,RetT> MbrFn;
		typedef MessageImpl<MbrFn,RetT,TargetT,SourceT > Super;
	public:
		MessageImpl2Mbr(const Obj &obj, Fn fn):Super(MbrFn(obj,fn)) {}
	};

	template<typename Fn, typename Args, typename RetT, typename TargetT, typename SourceT>
	class MessageImpl2: public
		MIf<MIsSame<Args,typename FunctionCall<Args>::T>::value,
			MessageImpl2Fn<Fn,Args,RetT,TargetT,SourceT>,
			MessageImpl2Mbr<Fn,Args,RetT,TargetT,SourceT> >::T {
		typedef typename MIf<MIsSame<Args,typename FunctionCall<Args>::T>::value,
				MessageImpl2Fn<Fn,Args,RetT,TargetT,SourceT>,
				MessageImpl2Mbr<Fn,Args,RetT,TargetT,SourceT> >::T Super;
	public:
		MessageImpl2(const Fn &fn, const Args &args):Super(fn,args) {}

	};


	template<typename Obj,typename Fn, typename Args, typename RetT, typename TargetT, typename SourceT>
	class MessageImpl3: public MessageImpl<
								FuncWithArgs<RetT, MemberFunctionAsFunctor<Obj,Fn,RetT>, Args>,
								RetT,TargetT,SourceT > {

		typedef MemberFunctionAsFunctor<Obj,Fn,RetT> MbrFn;
		typedef FuncWithArgs<RetT, MbrFn,Args> FnArgs;
		typedef MessageImpl<FnArgs,RetT,TargetT,SourceT > Super;
	public:
		MessageImpl3(const Obj &obj, Fn fn, typename Argument<Args>::T args)
			:Super(FnArgs(MbrFn(obj,fn),args)) {}
	};
}

namespace std {

template<typename A, typename B, typename C>
void swap(LightSpeed::Message<A,B,C> &a,LightSpeed::Message<A,B,C> &b) {
	a.swap(b);
}
}

#endif /* LIGHTSPEED_MESSAGES_MESSAGE_H_ */
