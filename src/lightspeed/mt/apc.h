/*
 * asp.h
 *
 *  Created on: 23.6.2013
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_APCIFC_H_
#define LIGHTSPEED_MT_APCIFC_H_
#include "atomic.h"


namespace LightSpeed {


	///Abstract class for APC message (Asynchronous Procedure Call)
	/**
	 * APC allows to execute messages in context of other thread.
	 *
	 * This class describes abstract APC message, which can be queued for
	 * execution into another thread. Thread can execute APC soon or later.
	 *
	 * Once message is processed, it is removed from the queue. Note always
	 * check APCServer's documentation, whether it deallocates message to
	 * free memory (APCThread doesn't support this)
	 *
	 * @see AbstractAPCServer, APCThread
	 */
	class IAPCServer;
	class AbstractAPC {
	public:
		AbstractAPC():nextAPC(0) {}

		///Execute APC - implement here
		virtual void run() throw() = 0;
		///APC has been removed without execution
		virtual void rejected() throw() {}

		virtual ~AbstractAPC() {}

	private:
		AbstractAPC *nextAPC;

		friend class IAPCServer;
	};


	///Basic interface to queue messages.
	/** if you want to give reference to APC server for queueing messages */
	class IAPCServer {
	public:
		///Queues message for execution
		/**
		 * @param apc pointer to an APC message. Message will be executed
		 * as soon as possible. If there are more waiting messages, they
		 * are queued into FIFO queue. Reference must be valid until
		 * message is executed. YOU CANNOT UNDO this call so don't allocate
		 * messages at stack. APCServers also doesn't deallocate executed
		 * messages,
		 *
		 * @return Return value can be ignored unless you are call base
		 *   function. Value true means that caller should be notified
		 *   about new message in the queue. Return false means, that
		 *   notification is not necesary, because somebody already did
		 *   (there are more messages in the queue still waiting to process)
		 *
		 */
		virtual bool execute(AbstractAPC *apc)  = 0;
		virtual ~IAPCServer() {}
	protected:
		void setNext(AbstractAPC *p,AbstractAPC *q) {p->nextAPC = q;}
		AbstractAPC *getNext(AbstractAPC *p) const {return p;}
	};


	///Abstract APC server supports queuing and executing message to easy build APC server
	/**
	 * This class is not really abstract, because it doesn't have abstract function. You
	 * can create instance of this class and enqueue messages or process messages "by hand".
	 *
	 * Note that there is no way how notify other side about new message - because there is no
	 * other side defined. AbstractAPCServer can be used in single thread application to implement
	 * easy execution queue
	 *
	 * See APCThread
	 */
	class AbstractAPCServer: public IAPCServer {
	public:
		AbstractAPCServer():queue(0) {}


		virtual bool execute(AbstractAPC *apc) {
			AbstractAPC *n = queue;
			do {
				setNext(apc,n);
			} while (lockCompareExchangePtr(&queue,n,apc));
			return n == 0;
		}

		///asks, whether there is any APC
		/**
		 * @retval true an APC is enqueued
		 * @retval false no APC in queue
		 */
		bool anyAPC() const {return queue != 0;}
		///Execute all APCs in the queue
		void executeQueue() throw() {
			while (executeOne()) ;
		}

		///Called before APC is executed
		/**
		 * Implementation should call base function, or handle call operation
		 * by self
		 *
		 * @param p pointer to APC to call
		 */
		virtual void onExecute(AbstractAPC *p) throw() {
			p->run();
		}

		///rejects all messages
		~AbstractAPCServer() {
			rejectAll();
		}

		///Reject all messages
		void rejectAll() {
			AbstractAPC *k = queue;
			while (k) {
				AbstractAPC *z = k;
				k = getNext(k);
				z->rejected();
			}
		}
	protected:
		AbstractAPC * volatile queue;

		bool executeOne() throw() {
			AbstractAPC *k = queue;
			if (k == 0) return false;
			if (getNext(k) == 0) {
				AbstractAPC *l = lockCompareExchangePtr<AbstractAPC>(&queue,k,0);
				if (l == k) {
					onExecute(k);
					return false;
				}
				k = l;
			}
			while (getNext(getNext(k)) != 0) {
				k = getNext(k);
			}
			AbstractAPC *z = getNext(k);
			setNext(k,0);
			onExecute(z);
			return true;

		}
	};


}


#endif /* LIGHTSPEED_MT_APCIFC_H_ */
