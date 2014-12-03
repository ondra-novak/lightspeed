/*
 * actionQueue.h
 *
 *  Created on: 8.4.2011
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ACTIONS_MSGQUEUE_H_
#define LIGHTSPEED_ACTIONS_MSGQUEUE_H_

#include "executor.h"
#include "../memory/allocPointer.h"
#include "../../mt/notifier.h"

namespace LightSpeed {

class Thread;
class ISleepingObject;


///Message Queue 
/**
 * MsgQueue class implements queue of messages that can be
 * processed in the worker thread. Message is object that 
 * contains code which will be processed in the worker thread.
 * Worker thread is waiting for the messages and when message
 * is enqueued, it gets the message and process the code in it.
 * Another messages can be enqueued into the queue during thread
 * processing the first message. After one message is finished
 * thread starts to process next queued message
 *
 * MsgQueue is implemented using minimal set of locks. It uses
 * interlocked operations to access shared queue. The queue is 
 * separated into two parts. First part (public queue) is accessed by outside
 * threads and it is used to place new messages. Second part (private queue)
 * is accessed by worker thread and contains messages to process
 * during next periods. Worker thread is able to take enqueued 
 * messages from the public queue atomically, so no locking
 * is needed. But this require to keep following rules
 *
 *  - there is only one worker thread. If you need more worker
 *  thread, process messages using parallel executor or use 
 *  additional mutexes
 *
 *  - other threads cannot access to private queue (there is only
 *  exception, while other threads can post priority messages
 *  directly to the private queue, which can be done atomically)
 *
 *  - other threads cannot request to cancel messages. Cancellation
 *  of the messages is possible, but it is implemented using special
 *  cancel-message. Then cancellation is made by worker thread after
 *  it finishes current message (which can be long). Cancellation 
 *  messages are priority messages, they are processed first before
 *  any ordinary message.
 *
 *  Priority messages can be posted by other threads. This messages
 *  are processed before any ordinary message. If there is
 *  more priority messages, they are processed in reverse order
 *  of incoming (next priority message has priority over all
 *  messages in the queue). So this allows to cancel priority
 *  messages too.
 *
 * Note that MsgQueue doesn't solve accessing to the memory manager
 * used to allocate space for the messages by multiple threads. 
 * By default, messages are created in outside thread but destroyed
 * in worker thread. To speedup this, you can use statically allocated
 * messages which don't need to be allocated and disposed.
 *
 * If you want to use this class, you need to inherit it and
 * implement worker thread and function notify(). Worker thread is
 * often sleeps at thread's sleep function and is waken up by
 * the notify() function. You can also use Notifier object to
 * implement waiting for the message
 *
 *
 * 
 */
 
class MsgQueue: public IExecutor {
public:
	class AbstractMsg;
protected:
	class MsgDestructor {
	public:
		void destroyInstance(AbstractMsg *inst) const {
			if (inst) inst->done();
		}
		AbstractMsg *createInstance(const AbstractMsg &) const {
			throwUnsupportedFeature(THISLOCATION,this,"createInstance");
			throw;
		}
	};

public:
	typedef AllocPointer<AbstractMsg, MsgDestructor> PMsg;

	///Abstract message
	/**
	 * Interface for abstract message. Also allows to create container
	 * using linked list where each message can contain pointer to next message
	 */
	class AbstractMsg {
	public:

		friend class MsgQueue;

		virtual ~AbstractMsg() {}

		///called when message is queued.
		/** This can be important when message wants to track each stage of
		 * processing. Function is called on caller thread
		 *
		 * @param msgQueue contains reference to queue where message has been queued
		 */
		virtual void queued(MsgQueue &) {}

		///Called to process message action
		/**
		 * @note, run() command cannot throw exceptions. You have to
		 * store exception in the content of message 
		 */
		 
		virtual void run() throw () = 0;
		///Called when message is rejected
		/**
		 * Message can be canceled or rejected by command flush.
		 */
		virtual void reject() throw() {}

		///Called when message is finished regardless on result
		/**
		 * Useful to destroy message
		 */

		virtual void done() throw() {delete this;}

		///isolates this message and returns pointer to next message with queue
		/**
		 * Function is not MT safe.
		 * @return pointer to next message
		 */
		AbstractMsg *isolate();


		AbstractMsg() {}


		PMsg next;

	};

	///Notification message.
	/** You can post this message to the message queue. When message
	 * is processed, object becomes signaled and wakeUp() event is
	 * invoked. Class inherits Notifier, so you can use this object as
	 * notification on various events in message queue
	 *
	 * Passing this message to the queue as ordinary message causes, 
	 * that notification arrives when worker thread processes all messages
	 * currently pending in the queue. Messages arrived after this
	 * message are not counted. See function syncToQueue()
	 *
	 * Passing this message to the queue as priority message causes
	 * that notification arrives when worker thread processes current message.
	 * If there is no message, notification arrives immediately. See waitForFinishMsg()
	 */

	class NotifyMsg: public AbstractMsg, public Notifier {
	public:
		NotifyMsg():rejected(false) {}
		NotifyMsg(ISleepingObject &forward):Notifier(forward),rejected(false) {}
		virtual void done() throw () {wakeUp(0);}
		virtual void run() throw() {}
		virtual void reject() throw() {rejected = true;}
		virtual void queued(MsgQueue &) {reset();rejected = false;}
		bool isRejected() const {return rejected;}
	protected:
		bool rejected;
	};

	///Flush all messages
	/** Allows to flush all messages by outside thread,
	   All messages in queue are marked as rejected and disposed

	   @see flush, wtFlush
	*/
	class FlushMsg: public NotifyMsg {
	public:
		FlushMsg() {}
		FlushMsg(ISleepingObject &fw):NotifyMsg(fw) {}

		virtual void queued(MsgQueue &msgQueue);
		virtual void run() throw();
	protected:
		MsgQueue *inqueue;

	};

	///Cancels message
	/** Message searches whole queue (public and private) for specified message
	 * and removes it from the queue. Message should be posted as priority message
	 * otherwise it has no meaning 
	 * 
	 * @see cancelMessage, wtCancelMessage
	 * 
	 */
	class CancelMsg: public FlushMsg {
	public:
		CancelMsg(AbstractMsg *msgToCancel);
		CancelMsg(AbstractMsg *msgToCancel, ISleepingObject &fw);

		virtual void run() throw();
	protected:
		AbstractMsg *toCancel;
	};

	///asynchronous cancellation	
	class CancelMsgAsync: public AbstractMsg, public DynObject {
	public:
		CancelMsgAsync(AbstractMsg *msgToCancel):inqueue(0),toCancel(msgToCancel) {}

		virtual void run() throw();
		virtual void done() throw();
		virtual void queued(MsgQueue &msgQueue);


	protected:
		MsgQueue *inqueue;
		AbstractMsg *toCancel;

		friend class MsgQueue;
	};

	///FinishMsg extends FlushMsg but includes call of Thread::finish()
	/** Use this message to notify worker thread, that work is done, it should
	 * exit immediately. This is best way to finish work before object is destroyed
	 *
	 * @see stopQueue
	 */
	class FinishMsg: public FlushMsg {
	public:
		FinishMsg() {}
		FinishMsg(ISleepingObject &fw):FlushMsg(fw) {}

		virtual void run() throw();
	};

	virtual void execute(const IExecAction &action);
	///Equivalent to flush()
	/**
	 * @param timeout specifies timeout to wait for flushing. Unsupported
	 *
	 * @note argument timeout is not supported, because function flush cannot leave
	 * waiting object in the queue. If argument is zero, function is equivalent flush(0) and
	 *  returns true, only if queue is empty. For other values, infinite waiting is performed
	 * and function returns true
	 * 
	 */
	 
	virtual bool stopAll(natural timeout = 0);

	virtual void join() {flush();}

	///Posts message to the queue
	virtual void postMessage(AbstractMsg *msg);

	///Posts priority message to the queue
	/**
	 * Priority messages are posted into priority queue and are processed
	 * before any other messages. Two priority messages are processed in reversed
	 * order.
	 *
	 * @param msg
	 */
	virtual void postPriorityMessage(AbstractMsg *msg);

	///Cancels message by sending cancellation message in the queue;
	/**
	 * @param cmsg cancellation message statically allocated (or on the stack)
	 * 
	 * @note you have to wait for completion. You cannot destroy instance of
	 * message before message is completed. You also cannot cancel the request.
	 */
	 
	void cancelMessage(CancelMsg &cmsg);
	
	///Cancels specified message synchronously
	/**
	 * @param msg message to cancel. Pointer can be invalid (can point to already deallocated
	 * message which had been deallocated in function done()). Function doesn't touch
	 * the memory if message is not presented in the queue.
	 *
	 * @param nowait specify true if you don't want to wait to cancellation. Use only with messages
	 * allocated in heap. Message needn't disappear until it is canceled or executed.
	 *
	 * @return in case of nowait=true, it returns pointer to cancellation message, which can be
	 * also canceled later. Otherwise, it return 0
	 *
	 * Note function is synchronous and will not return until message is not canceled.
	 * Because cancellation is performed by worker thread, function must wait to
	 * finishing current message before command can be carried out.
	 *
	 * @note You cannot specify timeout, because cancellation message cannot be canceled
	 *
	 * @warning Do not call function during processing message. This causes deadlock. Call wtCancelMessage() instead	 
	 * (exception - you can do this with nowait = true)
	 */
	 
	AbstractMsg *cancelMessage(AbstractMsg *msg, bool nowait = false);
	

	///Allows to outside thread to synchronize self with the queue state
	/** Message is enqueued at the end of the queue, and notifies 
	 * caller when whole queue is finished 
	 *
	 * @param msg instance of NotofyMsg allocated statically or at stack
	 *
	 * @note you have to wait for completion. You cannot destroy instance of
	 * message before message is completed. You can cancel message passing the
	 * pointer to the object into cancelMessage() function, but this still
	 * need waiting for cancellation.
	 */
	void syncToQueue(NotifyMsg &msg);

	///Synchronizes caller with message state
	/**
	 * Current thread will continue after all current messages in the queue will
	 * be processed
	 *
	 * Once thread starts waiting, operation cannot be interrupted
	 */
	 
	void syncToQueue();

	///Allows waiting for finishing current message
	/**
	 * @param msg statically allocated message and also Notifier which will
	 * be notified after current message is processed
	 *
	 * @note you have to wait for completion. You cannot destroy instance of
	 * message before message is completed. You cannot cancel this message
	 */
	 
	void waitForFinishMsg(NotifyMsg &msg);

	///Waits for current message
	/**
	 * Thread starts waiting until current message in the message queue is finished
	 *
	 *
	 * Once thread starts waiting, operation cannot be interrupted
	 */
	void waitForFinishMsg();

	///Flushes all message in the queue
	/**
	 * @param notify optionally you can specify pointer to object, which
	 * will be notified about finalizing this operation.
	 *
	 * AS other controlling operations, function flush() is always enqueued
	 * into the queue as priority message and it is carried out immediately
	 * once current processed message is finished.
	 * @note Waiting cannot be canceled. Don't invalidate waiting object
	 * before it is notified.

	 */
	void flush();

	///Stops message queue
	/** works same way as flush, but sets thread's exit flag causing that thread
	 * should exit immediately. Function waits to process FinishMsg so it can
	 * return before thread really exits. You still need join the thread
	 *
	 * Note that function expects that message queue uses worker thread to
	 * process messages and the thread is not shared by other components in program.
	 * Otherwise function can cause that thread exits without give chance to other
	 * component expect this.
	 *
	 * Function sets finish flag of worker thread. Don't use function if this is not
	 * expected operation 
	 */
	void finish();

	///Function is inherited from IJobControl
	/**
	 * @retval true message queue contains at least one message
	 * @retval false message queue is empty
	 */
	bool isRunning() const {return queueLen>0;}
	
	///ctor
	MsgQueue();
	///ctor
	MsgQueue(IRuntimeAlloc &rtAllocator);
	///dtor
	/** @note destructor expects that worker thread is terminated
	 * and there are no other threads that can use this object. 
	 * All messages remaining in the queue are flushed using function
	 * wtFlush(). If you want to terminate thread in the destructor
	 * call finish() function in child class 
	 */
	virtual ~MsgQueue();


	///Returns true, when message queue is empty
	bool empty() const;



	///Post function call to the message queue
	/**
	 * @param fn this can me Lambda function, function or functor
	 */
	 
	template<typename Fn>
	AbstractMsg *postFnCall(Fn fn);
	///Post function call to the message queue
	/**
	 * @param fn this can me Lambda function, function or functor
	 * @param reject function is called when request is rejected
	 */
	template<typename Fn1, typename Fn2>
	AbstractMsg *postFnCall(Fn1 fn, Fn2 reject);

	MsgQueue(const MsgQueue &other);

	
	///Retrieves pointer to currently processing message
	/**
	 * Function should be used only inside of message processing, not outside because result 
	 * can be invalid after return. 
	 *
	 * @return pointer to currently processed message, return 0 if none. The zero return doesn't mean
	 * that queue is empty. It can mean, that queue is preparing to start new message
	 */
	 
	AbstractMsg *getCurrentMessage() const;

	natural length() const {return queueLen;}

protected:


	class ExecMsg;
	
	///contains current length of the queue
	/** Every postMessage increases this counter while every processed message decreases this counter */
	atomic queueLen;

	///current queue - messages that will be processed soon
	/**
	 * queue is controlled by thread owning this object except
	 * function postPriorityMessage, which can be called from different
	 * thread. Priority messages are posted head of queue, which can
	 * cause changing this pointer, but not changing the linked list
	 * of messages
	 */
	 
	AbstractMsg *volatile curQueue;

	///new messages posted in normal priority
	/** messages are ordered in reverse order, list is
	 organized as stack. This allows to other threads write
	 to the stack without locking. When owner threads wants to
	 read next message, it can atomically pick current stack
	 of messages, reverse the order and append messages to curQueue list
	 */
	AbstractMsg *volatile newMessages;

	///Current processing message
	AbstractMsg * curMessage;

	///allocator to allocate messages and internal states. Allocator should be MT safe
	IRuntimeAlloc &rtAllocator;

	///Function is called to notify thread, that there is new message	 
	virtual void notify() = 0;

	///implements getting new message from queue
	virtual AbstractMsg *getNextMsg();
	///prepares queue moving newMessages to curMessages
	/** Function is virtual to allow override this
	  * to implement prioritizing */
	virtual void prepareQueue();

	///pumps one message
	/**
	 * @retval true message processed
	 * @retval false no message in the queue
	 */	 
	virtual bool pumpMessage();

	///Cancels specified message
	/** 
	 * Allows to cancel message by the worker thread
	 * @param msg message to cancel
	 * @note calling cancelMessage() in the worker thread causes 
	 * deadlock. 
	 */
	void wtCancelMessage(AbstractMsg *msg);

	void wtRemoveMessage(AbstractMsg *msg,AbstractMsg *nextMsg);

	///Flushes message queue inside worker thread
	/** Function can be called only by worker thread. It flushes message
	* queue marking all messages rejected
	* @note do not call flush() from the worker thread, it causes
	* deadlock 
	*/
	void wtFlush();

	///Similar to finish() but must be called from worker thread 
	void wtFinish();
	///Appends message list to the queue
	/** Function must be called from the worker thread */
	void appendQueue(AbstractMsg *q);
private:
	MsgQueue &operator=(const MsgQueue &other);
};



template<typename Fnb>
MsgQueue::AbstractMsg * MsgQueue::postFnCall( Fnb fnb )
{
	class MyMsg: public AbstractMsg, public DynObject {
	public:

		Fnb fnb;

		virtual void run() throw () {
			fnb();
		}


		MyMsg(Fnb fnb):fnb(fnb) {}
	};
	MyMsg *msg = new(rtAllocator) MyMsg(fnb);
	postMessage(msg);
	return msg;
}

template<typename Fn1,typename Fn2>
MsgQueue::AbstractMsg * MsgQueue::postFnCall( Fn1 fn1, Fn2 reject )
{
	class MyMsg: public AbstractMsg, public DynObject {
	public:

		virtual void run() throw () {
			fn1();
		}
		virtual void reject() throw() {
			fn2();
		}

		Fn1 fn1;
		Fn2 fn2;

		MyMsg(Fn1 fn1, Fn2 fn2):fn1(fn1), fn2(fn2) {}
	};
	MyMsg *msg = new(rtAllocator) MyMsg(fn1,reject);
	postMessage(msg);
	return msg;
}
}

#endif /* LIGHTSPEED_ACTIONS_MSGQUEUE_H_ */
