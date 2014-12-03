/*
 * apcthread.cpp
 *
 *  Created on: 23.6.2013
 *      Author: ondra
 */
#include "apcthread.h"

namespace LightSpeed {


bool APCThread::execute(AbstractAPC* apc) {
	bool k = apcImpl.execute(apc);
	if (k) wakeUp(0);
	return k;
}

void APCThread::enableAPC(bool enable) {
	apc_enabled = enable;
}

void APCThread::onExecute(AbstractAPC* p) throw() {
	enableAPC(false);
	p->run();
	enableAPC(true);
}

APCThread::APCThread():apc_enabled(true),apcImpl(this)
{
}

bool APCThread::impSleep(const Timeout& tm, natural& reason) {
	bool x = Thread::sleep(tm,reason);
	if (!x && apc_enabled) {
		apcImpl.executeQueue();
	}
	return x;
}


void LightSpeed::APCThread::APCh::onExecute(AbstractAPC* p) throw()
{
	owner->onExecute(p);
}
}
