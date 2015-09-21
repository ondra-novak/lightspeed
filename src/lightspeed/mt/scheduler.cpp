/*
 * scheduler.cpp
 *
 *  Created on: 16. 9. 2015
 *      Author: ondra
 */

#include "scheduler.h"

#include "../base/containers/sort.tcc"
#include "../base/exceptions/canceledException.h"
#include "../base/actions/promise.tcc"
#include "../base/actions/abstractDispatcher.tcc"
namespace LightSpeed {

Scheduler::Scheduler():eventHeap(eventMap),canFinish(false) {
}



namespace {
	struct CancelPromise {
	public:
		CancelPromise(Promise<void>::Result res):res(res) {}
		void operator()(const PException &e) {
			res.reject(e);
		}
	protected:
		Promise<void>::Result res;
	};

}
Promise<void> Scheduler::schedule(Timeout tm) {
	Promise<void> p;
	Promise<void>::Result r = p.createResult();
	Promise<void> q = dispatch(*this,&Scheduler::addEvent,Event(tm,r));
	q.whenRejectedCall(CancelPromise(r));
	return p;
}

void Scheduler::run() {
	running.close();
	canFinish = false;
	natural reason;
	while (!canFinish) {
		Timeout tm = getNextWait();
		this->implSleep(tm,reason);
	}
	cancelAllEvents();
	cancelQueue();
	cancelAllPromises();
	running.open();
}

void Scheduler::stop(bool async) {
	canFinish = true;
	if (async) running.wait(nil);
}

Scheduler::Event::Event(Timeout tm, Promise<void>::Result res):tm(tm),res(res) {}

bool Scheduler::Event::expired(const SysTime& ss) const {
	return tm.expired(ss);
}


bool Scheduler::Event::lessThan(const Event& ev) const {
	return tm > ev.tm;
}

void Scheduler::addEvent(const Event& ev) {
	eventMap.add(ev);
	eventHeap.push();
}

Timeout Scheduler::getNextWait() const {
	if (eventHeap.empty()) return nil;
	else return eventHeap.top().getTimeout();
}

bool Scheduler::fireExpired() {
	SysTime tm = SysTime::now();
	bool res = false;
	while (!eventHeap.empty() && eventHeap.top().expired(tm)) {
		Promise<void>::Result e = eventHeap.top().getPromise();
		eventHeap.pop();
		eventMap.trunc(1);
		e.resolve();
		res = true;
	}
	return res;
}

Scheduler::~Scheduler() {
	stop();
}

void Scheduler::cancelAllEvents() {
	for (natural i = 0; i < eventMap.length(); i++) {
		eventMap[i].getPromise().reject(CanceledException(THISLOCATION));
	}
}

Gate& Scheduler::getJoinObject() {
	return running;
}

const Timeout& Scheduler::Event::getTimeout() const {
	return tm;
}

Promise<void>::Result Scheduler::Event::getPromise() const {
	return res;
}



} /* namespace LightSpeed */

