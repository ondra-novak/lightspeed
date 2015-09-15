/*
 * dispatchThread.h
 *
 *  Created on: 15. 9. 2015
 *      Author: ondra
 */

#ifndef LIGHTSPEED_MT_DISPATCHTHREAD_H_
#define LIGHTSPEED_MT_DISPATCHTHREAD_H_
#include "thread.h"
#include "../base/actions/dispatcher.h"

namespace LightSpeed {

class DispatchThread: public AbstractDispatcher, public Thread {
public:
	DispatchThread();
	DispatchThread(IRuntimeAlloc *allocator);
	~DispatchThread();

	virtual void dispatchAction(AbstractAction *action);


protected:

	virtual bool impSleep(const Timeout &tm, natural &reason);

	AbstractAction * volatile queue;
	IRuntimeAlloc *allocator;

	void executeQueue();
	virtual IRuntimeAlloc &getActionAllocator();






};

} /* namespace LightSpeed */

#endif /* LIGHTSPEED_MT_DISPATCHTHREAD_H_ */
