/*
 * simpleMsgQueue.h
 *
 *  Created on: 23.3.2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_ACTIONS_SIMPLEMSGQUEUE_H_
#define LIGHTSPEED_ACTIONS_SIMPLEMSGQUEUE_H_
#include "executor.h"
#include "../containers/linkedList.h"
#include "../../mt/fastlock.h"

namespace LightSpeed {

///Simplified message queue
/** In compare to MsgQueue, this object is much simplified. It does
 * mean that it is very lightweight, but also it doesn't provide
 * all features.
 *
 * Object implements IExecutor (which is MT Safe, so anyone can
 * enqueue action)
 *
 * Enqueued actions CANNOT BE CANCELED. There is no method, how to
 * cancel action. However, you can neutralize already queued actions. ExecAction
 * can be packed with extra object which can do this job for you.
 *
 * Once somebody calls finish() or stopAll(), all messages are destroyed without
 * execution. There is no way how to notify messages about rejection. However,
 * you can detect this by using extra object packed with action, which can be used
 * to detect destruction without execution the action and perform some operation.
 *
 * Priority actions are supported through executePriority
 *
 *
 *
 *
 *
 */
class SimpleMsgQueue: public IExecutor {
public:

	///Enqueue and execute action
	/** Once action is enqueued, it cannot be canceled. You have
	 * to find way how to neutralize the action
	 *
	 * @param action to enqueue
	 */
	virtual void execute(const IExecAction &action);

	///Calls finish and join
	virtual bool stopAll(natural timeout = 0);

	///Removes all enqueued messages, doesn't affect current message
	virtual void finish();
	///Synchronizes execution with currently processing message
	virtual void join();
	///Returns true, if there message being processed
	virtual bool isRunning() const;

	///Executes action in priority
	/**
	 * Puts action to the beginning of the queue. Multiple priority messages
	 * are ordered in reversed order
	 * @param action to process in priority
	 */
	virtual void executePriority(const IExecAction &action);

protected:
	///linked link of actions
	LinkedList<ExecAction> actionList;
	///lock held during working with the queue
	FastLock insertLock;
	///lock held during execution a message
	mutable FastLock execLock;

	///process single message
	/**
	 * @retval true message processed
	 * @retval false no messages available
	 */
	bool pumpMessage();
	///Process all messages in the queue
	/**
	 * @retval true at least one message processed
	 * @retval false no messages available
	 */
	bool pumpAllMessages();

	///Process message in MT state
	/**
	 * Funcion picks and process single message without holding execLock.
	 * This is intended to multiple thread processing where there are two or more
	 * threads reading the same queue. Note that with this function, join()
	 * and stopall() cannot work anymore
	 * @return
	 */
	bool pumpMessageMT();

	virtual void onMessage();
};

}


#endif /* LIGHTSPEED_ACTIONS_SIMPLEMSGQUEUE_H_ */
