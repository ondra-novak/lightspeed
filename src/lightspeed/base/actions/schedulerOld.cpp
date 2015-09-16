#include "schedulerOld.h"

#include "../../mt/atomic.h"

namespace LightSpeed {


void OldScheduler::prepareQueue()
{
	natural curTime = getCurTime();
	//take first message in the "newMessages" stack
	AbstractMsg *p = newMessages,*n = 0;
	//move items out of the stack atomically (set pointer to zero)
	while (lockCompareExchangePtr(&newMessages,p,n) != p) {
		//when top of stack has been changed, repeat operation
		p = newMessages;
	}
	//if there are no messages - exit now;
	if (p == 0) return;

	//sort messages by time;
	AbstractMsg *sortedP = sortList(p,curTime);

	//detach current queue
	AbstractMsg *q = curQueue;
	while (lockCompareExchangePtr(&curQueue,q,(AbstractMsg *)0) != q) {
		q = curQueue;
	}

	//merge new messages with current;
	q = mergeSort(sortedP,q,curTime);

	//append merged and sorted messages to the current queue;
	appendQueue(q);
}

MsgQueue::AbstractMsg * OldScheduler::sortList( AbstractMsg * p,natural curTime )
{
	//before sorting, calculate count of messages
	natural cnt = 0;
	for (AbstractMsg *k = p; k; k=k->next) cnt++;
	//if there is zero or one message, no sorting is necessary
	if (cnt < 2) return p;
	//allocate temporary buffer at stack
	AbstractMsg **buff = (AbstractMsg **)alloca(cnt * sizeof(AbstractMsg *));	
	natural i = 0;
	//move all messages to the buffer
	for (AbstractMsg *k = p; k; ) {
		buff[i++] = k;
		AbstractMsg *z = k->next;
		k->next.detach();
		k = z;
	}

	//every slot in the buffer is linked list with one message
	//sorting is implemented using mergeSort two lists making one
	//for every cycle, count of slots is reduced to half
	//until there is one slot containing ordered list
	while (cnt>1) {
		//take each pair
		//i as index points to second of pair. Cycle stops
		//when there is no pair or single item
		for (i = 1 ; i < cnt; i+=2) {
			//merge pairs and store result
			buff[i>>1] = mergeSort(buff[i-1],buff[i],curTime);
		}
		//test for single item 
		//and move it only to corresponding position
		if (i == cnt) buff[i>>1] = buff[i-1];
		//reduce count by one half respecting even item
		cnt = (cnt + 1) >> 1;
	}
	//at the end,we have ordered list at slot 0
	return buff[0];
}

CompareResult OldScheduler::compareMsg( AbstractMsg * a, AbstractMsg * b, natural curTime )
{
	if (a == 0)
		if (b == 0)
			return cmpResultEqual;
		else
			return cmpResultGreater;
	else if (b == 0) 
		return cmpResultLess;
		
	//order messages by time, so messages without time must be processed immediately
	integer tma = 0, tmb = 0;
	//try to retrieve time from message a
	AbstractEvent *aa = dynamic_cast<AbstractEvent *>(a);
	if (NULL != aa)
	{
		//retrieve time relative to current time
		//this handles overflow of time
		tma = (integer)(aa->time - curTime);
	}

	//try to retrieve time from message a
	AbstractEvent *bb = dynamic_cast<AbstractEvent *>(b);
	if (NULL != bb)
	{
		//retrieve time relative to current time
		//this handles overflow of time
		tmb = (integer)(bb->time - curTime);
	}

	//compare times
	return tma < tmb?cmpResultLess:
		(tma > tmb?cmpResultGreater:cmpResultEqual);
}


MsgQueue::AbstractMsg * OldScheduler::mergeSort( AbstractMsg * a, AbstractMsg * b , natural curTime)
{
	//this variable holds head item
	AbstractMsg *h = 0;
	//this variable holds tail item
	AbstractMsg *k = 0;
	//select head item, choose near message
	if (compareMsg(a,b,curTime) == cmpResultLess) {
		//it will be a - set head and advance pointer
		h = a; a = a->next;
	} else { 
		//it will be b - set head and advance pointer
		h = b; b = b->next;
	}
	//set tail item
	k = h;
	//repeat while there is messages
	while (a || b) {
		//reset pointer to next message at tail
		k->next.detach();
		//compare messages and choose near one
		if (compareMsg(a,b,curTime) == cmpResultLess)  {
			//it will be a - append message
			k->next = a;
			//and thet advance pointer
			a = a->next;
		} else {
			//it will be b - append message
			k->next = b;
			//and advance pointer
			b = b->next;
		}
		k = k->next;
	}
	//all messages processed, reset next pointer at the tail
	k->next.detach();
	//return head
	return h;
}

	bool OldScheduler::pumpMessage()
	{
		do {
			if (getWaitTime() == 0) {
				return MsgQueue::pumpMessage();
			} else if (newMessages == 0) {
				return false;
			}
			prepareQueue();
		} while(true);
	}

	natural OldScheduler::getWaitTime() const
	{		
		AbstractMsg *a = curQueue;
		if (a) {
			AbstractEvent *e = dynamic_cast<AbstractEvent *>(a);
			if (NULL != e)
			{
				natural tm = e->time;
				integer rel = (integer)(tm - getCurTime());
				return (natural)(rel<0?0:rel);
			} else {
				return 0;
			}
		} else {
			return naturalNull;
		}
	}

	natural OldScheduler::getWaitTime()
	{
		const OldScheduler *k = this;
		prepareQueue();
		return k->getWaitTime();
	}


	OldScheduler::OldScheduler()
	{

	}

	OldScheduler::OldScheduler( IRuntimeAlloc &alloc )
		:MsgQueue(alloc)
	{

	}

	void OldScheduler::schedule( AbstractEvent *ev, natural interval )
	{
		ev->time = getCurTime()+interval;
		postMessage(ev);
	}

}
