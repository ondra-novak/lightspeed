/*
 * actionQueue.cpp
 *
 *  Created on: 8.4.2011
 *      Author: ondra
 */

#include <stdlib.h>
#include "../memory/stdAlloc.h"
#include "../../mt/atomic.h"
#include "../../mt/timeout.h"
#include "../../mt/sleepingobject.h"
#include "../memory/rtAlloc.h"
#include "../../mt/thread.h"
#include "msgQueue.h"
#include "../memory/dynobject.h"
#include "../../mt/notifier.h"


namespace LightSpeed {


class MsgQueue::ExecMsg: public AbstractMsg,public DynObject {
public:
	ExecMsg(const IExecAction &action, IRuntimeAlloc &alloc)
		:action(action.clone(alloc)) {}
	void run() throw () {
		action.deliver();
	}

protected:
	Message<void> action;
};


MsgQueue::AbstractMsg *MsgQueue::AbstractMsg::isolate() {
	return next.detach();
}

MsgQueue::MsgQueue()
:queueLen(0),curQueue(0),newMessages(0)
,rtAllocator(StdAlloc::getInstance()) {

}

MsgQueue::MsgQueue(IRuntimeAlloc &rtAllocator)
:queueLen(0),curQueue(0),newMessages(0)
,rtAllocator(rtAllocator) {

}

MsgQueue::MsgQueue( const MsgQueue &other )
	:rtAllocator(other.rtAllocator)
{

}

MsgQueue::~MsgQueue() {
	wtFlush();
}



void MsgQueue::prepareQueue() {

	//take first message in the "newMessages" stack
	AbstractMsg *p = newMessages,*n = 0;
	//move items out of the stack atomically (set pointer to zero)
	while (lockCompareExchangePtr(&newMessages,p,n) != p) {
		//when top of stack has been changed, repeat operation
		p = newMessages;
	}

	//now. "newMessages" stack is empty and all messages are in "p"
	//variable need to reverse stack to make queue
	PMsg revQ;

	//reverse stack
	while (p) {
		//isolate first message and take pointer to next
		AbstractMsg *q = p->isolate();
		//detach top of new stack and move it to next of new top
		p->next = revQ.detach();
		//make new top
		revQ = p;
		//move itertator to next item
		p = q;
	}

	//append new queue to the current
	appendQueue(revQ.detach());
}
void MsgQueue::appendQueue(AbstractMsg *p) {
	//current queue can be expanded by different threads only at the begining
	//so this test must be atomical
	//if curQueue is empty, make new queue current - atomically
	if (lockCompareExchangePtr(&curQueue,(AbstractMsg *)0,p) == 0)
		//success - return
		return;
	else {
		//we are here now, because curQueue is not empty
		//only this thread can expand queue at the end, so this
		//is the safe operation
		//take current top - I know, that top can changed, but this item
		//is still valid
		AbstractMsg *q = curQueue;
		//find end of queue
		while (q->next != nil) q = q->next;
		//replace last "next" null pointer with new queue
		q->next = p;
	}

}
void MsgQueue::postMessage(AbstractMsg *msg) {
	//you cannot post NULL message
	if (msg == 0 || msg->next != nil) return;
	//notify message, that is queued
	msg->queued(*this);
	//set next by current top
	msg->next = newMessages;
	//atomically set new top
	while (lockCompareExchangePtr(&newMessages,msg->next.get(),msg) != msg->next.get()) {
		//if not successfull (current top has been changed meanwhile)
		//detach next
		msg->next.detach();
		//and try new top
		msg->next = newMessages;
		//try agaiin
	}
	lockInc(queueLen);
	//notify worker thread about new message in the queue
	notify();

}

void MsgQueue::postPriorityMessage(AbstractMsg *msg) {
	//you cannot post NULL message
	if (msg == 0 || msg->next != nil) return;
	//notify message, that is queued
	msg->queued(*this);
	msg->next = curQueue;
	while (lockCompareExchangePtr(&curQueue,msg->next.get(),msg) != msg->next.get()) {
		msg->next.detach();
		msg->next = curQueue;
	}
	lockInc(queueLen);
	notify();
}

void MsgQueue::notify() {

}

MsgQueue::AbstractMsg *MsgQueue::getNextMsg() {

		
	//if there are no message in the curQueue, prepare queue from newMessages
	if (curQueue == 0) prepareQueue();
	//take new message from top of curQueue
	AbstractMsg *m = curQueue;
	//if empty, return 0;
	if (m == 0) return 0;
	//atomically remove the message from the queue
	while (lockCompareExchangePtr(&curQueue,m,m->next.get()) != m) {
		//update top most message, when failure
		m = curQueue;
	}
	//now, top of curQueue is updated - isolate the message
	m->isolate();

	lockDec(queueLen);

	//return message
	return m;
}


void MsgQueue::execute(const IExecAction &action) {

	postMessage(new(rtAllocator) ExecMsg(action,rtAllocator));


}

bool MsgQueue::pumpMessage() {
	PMsg m = getNextMsg();
	if (m == nil) return false;
	curMessage = m;
	m->run();
	curMessage = 0;
	return true;
}


void MsgQueue::flush() {
	FlushMsg msg;
	postPriorityMessage(&msg);
	while (!msg) Thread::sleep(nil);
}

MsgQueue::AbstractMsg * MsgQueue::cancelMessage(AbstractMsg *msg, bool nowait) {
	if (nowait) {
		CancelMsgAsync *c = new(rtAllocator) CancelMsgAsync(msg);
		postPriorityMessage(c);
		return c;
	} else {
		CancelMsg m(msg);
		postPriorityMessage(&m);
		while (!m) Thread::sleep(nil);
		return 0;
	}
}

void MsgQueue::cancelMessage( CancelMsg &cmsg )
{
	postPriorityMessage(&cmsg);
}



void MsgQueue::wtCancelMessage(AbstractMsg *msg)  {

	if (msg == 0) return;
	prepareQueue();

	AbstractMsg *p,*n = 0;
	do {
		p = curQueue;
		if (p == msg) {
			if (lockCompareExchangePtr(&curQueue,p,p->next.get()) == p) {
				p->isolate();
				p->reject();
				p->done();
				lockDec(queueLen);
				return;
			} 
		}
	} while (p == msg);

	if (p == 0) return ;

	n = p;
	p = p->next;
	while (p) {
		if (p == msg) {
			n->isolate();
			n->next = p->isolate();
			p->reject();
			p->done();
			lockDec(queueLen);
			return;
		} else {
			n = p;
			p = p->next.get();
		}
	}
}


bool MsgQueue::empty() const {
	return newMessages == 0 && curQueue == 0;
}

bool MsgQueue::stopAll( natural timeout /*= 0*/ )
{
	if (timeout == 0) {
		return empty();
	} else {	
		flush();
		return true;
	}
}

void MsgQueue::syncToQueue( NotifyMsg &msg )
{
	postMessage(&msg);
}

void MsgQueue::syncToQueue()
{
	NotifyMsg msg;
	postMessage(&msg);
	while (!msg) Thread::sleep(nil);
}

void MsgQueue::waitForFinishMsg( NotifyMsg &msg )
{
	postPriorityMessage(&msg);
}

void MsgQueue::waitForFinishMsg()
{
	NotifyMsg msg;
	postPriorityMessage(&msg);
	while (!msg) Thread::sleep(nil);

}

void MsgQueue::finish()
{
	FinishMsg msg;
	postPriorityMessage(&msg);
	while (!msg) Thread::sleep(nil);
}

void MsgQueue::wtFlush()
{
	prepareQueue();
	PMsg m = getNextMsg();
	while (m != nil) {
		m->reject();
		m = getNextMsg();
	}
}

void MsgQueue::wtFinish()
{
	Thread::current().finish();
	wtFlush();
}

MsgQueue::AbstractMsg * MsgQueue::getCurrentMessage() const
{
	return curMessage;
}


void MsgQueue::FlushMsg::queued( MsgQueue &msgQueue )
{
	inqueue = &msgQueue;
	NotifyMsg::queued(msgQueue);
}

void MsgQueue::FlushMsg::run() throw()
{
	inqueue->wtFlush();
	NotifyMsg::run();
}

MsgQueue::CancelMsg::CancelMsg( AbstractMsg *msgToCancel )
	:toCancel(msgToCancel)
{

}

MsgQueue::CancelMsg::CancelMsg( AbstractMsg *msgToCancel, ISleepingObject &fw )
	:FlushMsg(fw),toCancel(msgToCancel)
{

}

void MsgQueue::CancelMsg::run() throw()
{
	inqueue->wtCancelMessage(toCancel);
	NotifyMsg::run();
}

void MsgQueue::FinishMsg::run() throw()
{
	Thread::current().finish();
	FlushMsg::run();
}


void MsgQueue::CancelMsgAsync::run() throw()
{
	if (inqueue) inqueue->wtCancelMessage(toCancel);
}

void MsgQueue::CancelMsgAsync::done() throw()
{
	delete this;
}

void MsgQueue::CancelMsgAsync::queued( MsgQueue &msgQueue )
{
	inqueue = &msgQueue;
}
}


