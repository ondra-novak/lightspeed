/*
 * distributor.h
 *
 *  Created on: 7.11.2010
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ACTIONS_DISTRIBUTOR_H_
#define LIGHTSPEED_ACTIONS_DISTRIBUTOR_H_

#include "../containers/linkedList.h"
#include "../sync/nulllock.h"
#include "message.h"
#include "../memory/pointer.h"
#include "../sync/syncobj.h"

namespace LightSpeed {

	///Turns off stages in Distributor
	/**
	 * @see Distributor, DistributorDefCfg
	 */
	class DistributorNoStages {
	public:
		bool operator==(const DistributorNoStages &) const {return true;}
		bool operator!=(const DistributorNoStages &) const {return false;}
	};

	///Distributes events to group of listeners
	/**
	 * Distributor solves problems with sending notifications to the
	 * listeners. Because distributing is made in single thread,
	 * only one listener can process notification at once. If you need
	 * process notification using multiple thread, each listener must
	 * create thread and return control to the distributor immediately
	 *
	 * Distributor also supports multiple requests during distribution. Listeners
	 * are allowed to produce request to distribution. Each request is
	 * queued and listener must wait to finish distribution, while
	 * the original request is finished first and then queued request is distributed.
	 * Distributor is able to distribute any count of requests, limit
	 * is defined by the stack size, because queue is partially implemented on the stack.
	 *
	 * To support multithreaded environment, distributor supports multiple queues
	 * each for different thread. You can define StageSelector that allows
	 * to identify stage. Stage contains current state of distribution. Stages
	 * are independent to each other. Define smart class to choose stage, and
	 * distributor can work independently for each thread. StageSelector is
	 * not limited for threads, you can use it for generally in any  situation
	 * when you need to have multiple distribution states
	 *
	 *
	 * @tparam Ifc class of pointer type to interface that will implement
	 * messages sent through distributor. You can specify pointer directly, or
	 * you can specify a smart pointer.
	 *
	 * @tparam Config configuration for distributor. See DistributorDefCfg to
	 * see documentation for configuration
	 *
	 */
	template<typename Ifc,
			 typename StageSel = DistributorNoStages,
			 typename Lock = NullLock>
	class Distributor: public SyncObj< Lock> {


	public:

		typedef SyncObj<Lock> Super;

		///Message envelope
		/** It contains information about delivering message
		 * required when listener needs some additional
		 * informations from the distributor
		 */
		struct Envelop {
			///Contains reference to distributor, which posted this message
			Distributor &dist;
			///Contains reference to stage used to queue this message
			const StageSel &stage;
			///pointer to sender. Can be nil, if sender has not been specified
			Pointer<Ifc> sender;
			///
			Envelop (Distributor &d,const StageSel &s, Pointer<Ifc> sender)
				:dist(d),stage(s), sender(sender) {}
		};


		///Defines type of message
		/** It will accepted messages for specified pointer or smart pointer Ifc
		 * and additionally, Envelop struct can be included into message call.
		 * This allows to use messages with zero, one or two parameters, where
		 * first parameter of two contains reference to Envelop
		 */
		typedef Message<void, Ifc, const Envelop &> Msg;
		typedef typename Msg::Ifc IMessage;

		///Constructs distributor
		/**
		 * @param exitMsg specifies exit message. Exit message is message, which
		 * is sent to the all listeners when distributor is being destroyed.
		 * Pointer must be valid during whole lifetime of distributor. If 0 specified,
		 * no exit message is defined.
		 *
		 * @see sendExitMsg
		 *
		 */
		Distributor(const IMessage *exitMsg = 0):exitMsg(exitMsg),stages(nullFact) {}

		Distributor(const IMessage *exitMsg, const Lock &lock, NodeAlloc factory)
			:Super(lock),exitMsg(exitMsg),listeners(factory) {}


		~Distributor() {
			sendExitMsg();
			removeAll();
		}


		///adds new listener
		/**
		 * @param ptr pointer or smart pointer to new listener
		 */
		void add(Ifc ptr);

		///adds new listener to the front of list
		/**
		 * @param ptr pointer or smart pointer to new listener
		 */
		void priorityAdd(Ifc ptr);

		///Checks, whether listener is added
		/**
		 * @param ptr pointer or smart pointer to listener
		 * @return count of references in the list. There will be
		 * zero, if listener has not been added, or 1 if there is one
		 * instance. Because you are allowed to add listener multiple
		 * times, function can return values above 1 in that case
		 **/
		natural isAdded(Ifc ptr) const;

		///Removes listener
		/**
		 * Removes first listener from the distributor
		 * @param ptr pointer or smart pointer to the listener
		 * @retval true removed
		 * @retval false not found
		 */
		bool remove(Ifc ptr);

		///Removes all listeners
		/**
		 * Note function will not send "exit message". Listeners
		 * are removed silently
		 */
		void removeAll();

		///Sends message to the default stage
		/**
		 * @param msg reference to the message to send
		 * Function will not return until message is processed
		 * by all listeners
		 *
		 * Function can throw any exception, which has been produced
		 * by any listener during processing. Exception always stops
		 * processing all messages in the all stages
		 */
		void send(const Msg &msg);

		///Sends message to the specified stage
		/**
		 * @param msg reference to the message to send
		 * Function will not return until message is processed
		 * by all listeners
		 *
		 * @param stage defines stage.
		 *
		 * Function can throw any exception, which has been produced
		 * by any listener during processing. Exception always stops
		 * processing all messages in the all stages
		 */
		void send(const Msg &msg, const StageSel &stage);

		///Sends message to the default stage specifying the sender
		/**
		 * @param sender Defines sender. Sender is listener, which
		 * produces this request. During processing the request,
		 * sender is skipped. To retrieve current sender, use
		 * getSender() function
		 *
		 * @param msg reference to the message to send
		 * Function will not return until message is processed
		 * by all listeners
		 *
		 * Function can throw any exception, which has been produced
		 * by any listener during processing. Exception always stops
		 * processing all messages in the all stages
		 */

		void send(Ifc sender, const Msg &msg);

		///Sends message to the default stage specifying the sender
		/**
		 * @param sender Defines sender. Sender is listener, which
		 * produces this request. During processing the request,
		 * sender is skipped. To retrieve current sender, use
		 * getSender() function
		 *
		 * @param msg reference to the message to send
		 * Function will not return until message is processed
		 * by all listeners
		 *
		 * @param stage stage associated with current request
		 *
		 * Function can throw any exception, which has been produced
		 * by any listener during processing. Exception always stops
		 * processing all messages in the all stages
		 */
		void send(Ifc sender, const Msg &msg, const StageSel &stage);

		///Sends exit message
		/**
		 * Exit message is defined in the constructor. It used
		 * to notify listeners, that distributor is destroying itself
		 * or removing all listeners. Function is always called by
		 * the destructor.
		 *
		 * @note while message is distributed, all listeners must
		 * watch out for std::uncaught_exception before they want
		 * to throw the exception. That because this function
		 * is called during destruction of distributor. If listener
		 * throws exception, function doesn't stop distribution, all
		 * listeners are notified, but std::uncaught_exception is
		 * set to 1 and throwing additional exception will cause
		 * double-exception error.
		 *
		 * @note exit message cannot be stopped. Invoking stop()
		 * can cause to stop default stage, but not distribution
		 * of exit message, even if it appears that default stage is
		 * used for distribution
		 *
		 * @note sending message into default stage will pause distribution
		 * of exit message until whole stage is finished. Same
		 * behavior, if finish() is used during distribution. Note that
		 * finish() will not finish the exit message stage. You cannot
		 * postpone processing of exit message using the finish() function.
		 */
		void sendExitMsg();

		///Finishes stage
		/**
		 * In current context finishes the specified stage. It finishes all
		 * queued requests for the stage including requests
		 * added by any listener called during distributon,
		 *
		 * @param stage specifies stage to finish
		 */
		void finish(const StageSel &stage);

		///Finishes default stage
		/**
		 * In current context finishes the default stage. It finishes all
		 * queued requests for the current stage including requests
		 * added by any listener called during distributon,
		 */
		void finish();


		///Retrieves current listener for the stage
		/**
		 * @param stage selected stage
		 * @return listener reference
		 * @exception NoListenerException if stage doesn't processing the messages
		 */
		Ifc getCurrent(const StageSel &stage) const;

		///Retrieves current listener for default stage
		/**
		 * @return listener reference
		 * @exception NoListenerException if stage doesn't processing the messages
		 */
		Ifc getCurrent() const;

		///Retrieves current sender for the stage
		/**
		 * @param stage selected stage
		 * @return listener reference
		 * @exception NoListenerException if stage doesn't processing the messages or sender is not defined
		 */
		Ifc getSender(const StageSel &stage) const;

		///Retrieves current sender for default stage
		/**
		 * @return listener reference
		 * @exception NoListenerException if stage doesn't processing the messages or sender is not defined
		 */
		Ifc getSender() const;

		///True if distributor is empty
		bool empty() const;

		///Count of listeners
		natural size() const;

		void clear() {removeAll();}

		///Stops specified stage
		/** Prevents continuation of broadcasting for specified stage
		 * @param stage specifies stage
		 *
		 * @note stops all request for specified stage. When listener returns
		 * control, distributor will give control to all listeners which
		 * waiting to finish distributing, but will not continue for other
		 * listeners
		 */
		void stop(const StageSel &stage);

		///Stops default stage
		/** Prevents continuation of broadcasting for default stage
		 *
 		 * @note stops all request for specified stage.
		 *
		 * @note stops all request for specified stage. When listener returns
		 * control, distributor will give control to all listeners which
		 * waiting to finish distributing, but will not continue for other
		 * listeners
		 */
		void stop();

	protected:
		///Linked list type - list of interfaces allocated using factory
		typedef LinkedList<Ifc> Listeners;
		///Descriptor of single stage
		/** Descriptors are created at the stack and caller
		 * must not exit, until added descriptor is finished. Because
		 * function finish will always process whole stage until all
		 * request are removed, this is not problematic condition
		 */
		struct Stage {
			///reference to the message
			const Msg &msg;
			///stage identifier
			StageSel num;
			///Current position in the stage
			typename Listeners::Iterator pos;
			///Sender
			Ifc *sender;
			///Constructor
			Stage(const Msg &msg, StageSel num, const Listeners &lst, Ifc *sender)
				:msg(msg), num(num),pos(lst.getFwIter()),sender(sender) {}
		};

		///Factory, which disabled deallocation during item removing from the linked list
		/** This is need because items are allocated at stack and they will be
		 * deallocated automatically.
		 */
		class NullFactory: public IRuntimeAlloc {
		public:
			LIGHTSPEED_CLONEABLECLASS;

			void dealloc(void *, natural) {}
			void *alloc(natural ) {throwUnsupportedFeature(THISLOCATION,this,"alloc");return 0;}
			void *alloc(natural , IRuntimeAlloc *&) {throwUnsupportedFeature(THISLOCATION,this,"alloc");return 0;}
		};

		typedef LinkedList<Stage> Stages;
		typedef typename Stages::ListItem StageItem;

		const IMessage *exitMsg;
		Pointer<Stage> exitMsgStage;
		Listeners listeners;
		NullFactory nullFact;
		Stages stages;

		typedef typename Super::Sync Sync;
		typedef typename Super::RevSync RevSync;


		void _finish(const StageSel &stage);
		void _stop(const StageSel &stage);
		typename Stages::Iterator findStage(const StageSel &sel) const ;
	};


	class DistributorException: public virtual Exception {
	public:
		DistributorException() {}
	};

	class NoListenerException: public DistributorException{
	public:
		NoListenerException(const ProgramLocation &loc)
			:Exception(loc) {}

		LIGHTSPEED_EXCEPTIONFINAL;

		static const char *msgText;
		virtual void message(ExceptionMsg &msg) const  {
			msg(msgText);
		}
	};


}  // namespace LightSpeed



#endif /* LIGHTSPEED_ACTIONS_DISTRIBUTOR_H_ */

